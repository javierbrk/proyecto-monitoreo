#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <WiFiManager.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <esp_ota_ops.h>
#include "version.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "sendDataGrafana.h"
#include "createGrafanaMessage.h"
#include "constants.h"
#include "globals.h"
#include "endpoints.h"
#include "configFile.h"
#include "otaUpdater.h"
#include "actuators/RelayManager.h"

#ifdef SENSOR_MULTI
  #include "SensorManager.h"
  SensorManager sensorMgr;
  // Wrapper function for endpoints.cpp to access sensors without including full header
  std::vector<ISensor*>& getSensorList() { return sensorMgr.getSensors(); }
#else
  #include "sensors/SensorFactory.h"
#endif

#ifdef ENABLE_RS485
  #include "RS485Manager.h"
  RS485Manager rs485;
#endif

#ifdef ENABLE_ESPNOW
  #include "ESPNowManager.h"
  ESPNowManager espnowMgr;
#endif

RelayManager relayMgr;

// Forward declarations for new endpoints
void handleRelayList();
void handleRelayToggle();

unsigned long lastUpdateCheck = 0;
unsigned long lastSendTime = 0;

#ifdef ENABLE_ESPNOW
// Mesh data buffer structure to avoid HTTP calls from WiFi interrupt context
struct MeshDataBuffer {
  uint8_t senderMAC[6];
  char sensorId[32] ;
  float temp;
  float hum;
  float co2;
  uint32_t seq;
  bool valid;
};

const int MESH_BUFFER_SIZE = 10;
MeshDataBuffer meshBuffer[MESH_BUFFER_SIZE];
volatile int meshBufferHead = 0;
volatile int meshBufferTail = 0;
#endif

#ifndef UNIT_TEST

#ifdef ENABLE_ESPNOW

// Callback to enqueue mesh data (gateway only)
// IMPORTANT: Runs in WiFi interrupt context - must not call HTTP/blocking functions
void onMeshDataReceived(const uint8_t* senderMAC, float temp, float hum, float co2, uint32_t seq, const char* sensorId) {
  // Calculate next buffer position
  int nextHead = (meshBufferHead + 1) % MESH_BUFFER_SIZE;
  Serial.println(".");

  // Check if buffer is full
  if (nextHead == meshBufferTail) {
    Serial.println("[MESH] ✗ Buffer full, dropping data");
    return;
  }
 Serial.println(".");
  // Store data in buffer
  memcpy(meshBuffer[meshBufferHead].senderMAC, senderMAC, 6);

  if (sensorId != nullptr) {
    strncpy(meshBuffer[meshBufferHead].sensorId, sensorId, sizeof(meshBuffer[meshBufferHead].sensorId) - 1);
    meshBuffer[meshBufferHead].sensorId[sizeof(meshBuffer[meshBufferHead].sensorId) - 1] = '\0';
  } else {
    strcpy(meshBuffer[meshBufferHead].sensorId, "unknown");
  }  
  meshBuffer[meshBufferHead].temp = temp;
  meshBuffer[meshBufferHead].hum = hum;
  meshBuffer[meshBufferHead].co2 = co2;
  meshBuffer[meshBufferHead].seq = seq;
  meshBuffer[meshBufferHead].valid = true;
 Serial.println(".");
  // Update head pointer (atomic for single-writer scenario)
  meshBufferHead = nextHead;
 Serial.println(".");
  //Serial.printf("[ESP-NOW] Data buffered from sensor %d (seq=%lu)\n",
               // senderMAC[5], seq);
}

String detectRole(const JsonDocument& config) {
  Serial.println("\n[→ INFO] Auto-detectando rol del dispositivo...");

  // 1. Check if WiFi is connected
  if (!wifiManager.isOnline()) {
    Serial.println("  └─ Sin conexión WiFi → SENSOR mode");
    return "sensor";
  }

  Serial.println("  └─ WiFi conectado, verificando acceso a Grafana...");

  // 2. Test Grafana connectivity
  String grafanaUrl = config["grafana_ping_url"] | "http://192.168.1.1/ping";

  HTTPClient http;
  http.begin(grafanaUrl);
  http.setTimeout(3000);  // 3 second timeout

  int httpCode = http.GET();
  http.end();

  if (httpCode > 0) {
    Serial.printf("  └─ Grafana accesible (HTTP %d) → GATEWAY mode\n", httpCode);
    return "gateway";
  } else {
    Serial.printf("  └─ Grafana inaccesible (error %d) → SENSOR mode\n", httpCode);
    return "sensor";
  }
}
#endif

void printBanner() {
  Serial.println("\n\n");
  Serial.println("  ╔═══════════════════════════════════════════════════╗");
  Serial.println("  ║                                                   ║");
  Serial.println("  ║      *---*         ALTERMUNDI          *---*      ║");
  Serial.println("  ║     /     \\                           /     \\     ║");
  Serial.println("  ║    *   *   *    Proyecto Monitoreo  *   *   *    ║");
  Serial.println("  ║     \\ | | /         Sensores          \\ | | /     ║");
  Serial.println("  ║      *---*                             *---*      ║");
  Serial.println("  ║                                                   ║");
  Serial.println("  ║   La pata tecnológica de ese otro mundo posible  ║");
  Serial.println("  ╚═══════════════════════════════════════════════════╝");
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(500);  // Esperar estabilización serial

  printBanner();

  Serial.println("[→ INFO] Iniciando sistema...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("  INICIALIZACIÓN DEL SISTEMA");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

  if (!SPIFFS.begin(true)) {
    Serial.println("[✗ ERR ] No se pudo montar SPIFFS");
  } else {
    Serial.println("[✓ OK  ] SPIFFS montado correctamente");
  }

  createConfigFile();

  // Load configuration ONCE for all modules
  JsonDocument config = loadConfig();

  Serial.println("\n[→ INFO] Configuración cargada:");
  String configOutput;
  serializeJsonPretty(config, configOutput);
  Serial.println(configOutput);
  Serial.println("[✓ OK  ] Configuración impresa");

  #ifdef ENABLE_RS485
    
    Serial.println("\n[→ INFO] Configurando RS485...para envios seriales");
    if (!config["rs485_enabled"] | false) {
      Serial.println("[→ INFO] RS485/Modbus deshabilitado en configuración");
    }
    else {
      // 1. Initialize ModbusManager first (Singleton owner of the bus)
      ModbusManager::getInstance().begin(config["rs485_rx"], config["rs485_tx"], config["rs485_de"], config["rs485_baud"]);

      // 2. Initialize RS485Manager (will reuse ModbusManager's serial if available)
      rs485.init(config["rs485_rx"], config["rs485_tx"], config["rs485_baud"], config["rs485_de"], config["rs485_de"]);
      
      Serial.println("[✓ OK  ] RS485/Modbus habilitado");
      delay(100);
    }
  #endif

  
  Serial.println("\n[→ INFO] Inicializando sensores...");
  #ifdef SENSOR_MULTI
    sensorMgr.loadFromConfig(config);
    int sensorCount = sensorMgr.getSensorCount();
    Serial.printf("[✓ OK  ] Modo multi-sensor: %d sensor%s activo%s\n",
                  sensorCount, sensorCount != 1 ? "es" : "", sensorCount != 1 ? "s" : "");

    // Listar sensores activos
    for (auto* s : sensorMgr.getSensors()) {
      if (s && s->isActive()) {
        String sensorId = sensorMgr.getSensorId(s);
        Serial.printf("  └─ %s\n", sensorId.c_str());
      }
    }

  // Initialize Relays
  Serial.println("\n[→ INFO] Inicializando Relés...");
  relayMgr.loadFromConfig(config);
  Serial.printf("[✓ OK  ] %d relés configurados\n", relayMgr.getRelays().size());
     // Initialize relays to verify connection
    for (auto* r : relayMgr.getRelays()) {
        if (r) r->init();
    }
  #else
    sensor = SensorFactory::createSensor();
    if (sensor) {
      if (sensor->init()) {
        Serial.printf("[✓ OK  ] Sensor %s inicializado\n", sensor->getSensorType());
      } else {
        Serial.printf("[✗ ERR ] Error inicializando %s\n", sensor->getSensorType());
      }
    } else {
      Serial.println("[✗ ERR ] No se pudo crear el sensor");
    }
  #endif

  clientSecure.setInsecure(); 

  server.on("/actual", HTTP_GET, handleMediciones);
  server.on("/config", HTTP_GET, handleConfiguracion);
  server.on("/config", HTTP_POST, habldePostConfig);
  server.on("/config/reset", HTTP_POST, handleConfigReset);
  server.on("/data", HTTP_GET, handleData);
  server.on("/calibrate-scd30", HTTP_GET, handleSCD30Calibration);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/restart", HTTP_POST, handleRestart);
  
  // Relay endpoints
  server.on("/api/relays", HTTP_GET, handleRelayList);
  server.on("/api/relay/toggle", HTTP_POST, handleRelayToggle);

  #ifdef ENABLE_ESPNOW
    server.on("/espnow/status", HTTP_GET, handleESPNowStatus);
  #endif

  // Serve favicon from SPIFFS
  server.on("/favicon.svg", HTTP_GET, []() {
    File file = SPIFFS.open("/favicon.svg", "r");
    if (!file) {
      server.send(404, "text/plain", "Favicon not found");
      return;
    }
    server.streamFile(file, "image/svg+xml");
    file.close();
  });

    // Add handler for undefined routes
  server.onNotFound([]() {
      // Redirect all undefined routes to root page
      Serial.println("Redirecting to root page for undefined route");
      Serial.printf("Requested route: %s\n", server.uri().c_str());
      server.sendHeader("Location", "/", true);
      server.send(302, "text/plain", "");
  });



  server.enableCORS(true);

  Serial.println("\n[→ INFO] Configurando WiFi Manager...");
  wifiManager.setConnectionTimeout(15000);
  wifiManager.setMaxRetries(8);
  wifiManager.setValidationTimeout(30000);
  wifiManager.init(&server);
  Serial.println("[✓ OK  ] WiFi Manager inicializado");



  #ifdef ENABLE_ESPNOW
    Serial.println("\n[→ INFO] Configurando ESP-NOW...");
    bool espnowEnabled = config["espnow_enabled"] | false;

    if (espnowEnabled) {
      // Auto-detect role or use forced mode
      String forcedMode = config["espnow_force_mode"] | "";
      String espnowMode;

      if (forcedMode != "") {
        Serial.printf("[→ INFO] Modo forzado: %s\n", forcedMode.c_str());
        espnowMode = forcedMode;
      } else {
        // Auto-detection based on connectivity
        espnowMode = detectRole(config);
      }

      uint8_t espnowChannel = config["espnow_channel"] | 1;

      // Validate channel is in valid range (1-13)
      if (espnowChannel < 1 || espnowChannel > 13) {
        Serial.printf("[⚠ WARN] Canal inválido %d, usando canal 1\n", espnowChannel);
        espnowChannel = 1;
      }

      Serial.printf("[→ INFO] Modo ESP-NOW: %s (canal %d)\n", espnowMode.c_str(), espnowChannel);

      if (espnowMode == "gateway") {
        // Gateway: use current WiFi channel if connected, otherwise use configured channel
        if (wifiManager.isOnline()) {
          uint8_t wifiChannel = WiFi.channel();
          if (wifiChannel >= 1 && wifiChannel <= 13) {
            espnowChannel = wifiChannel;
            Serial.printf("[→ INFO] Gateway usando canal WiFi: %d\n", espnowChannel);
          } else {
            Serial.printf("[⚠ WARN] Canal WiFi inválido (%d), usando canal configurado: %d\n", wifiChannel, espnowChannel);
          }
        } else {
          Serial.printf("[→ INFO] WiFi no conectado, gateway usando canal configurado: %d\n", espnowChannel);
        }
      }

      if (espnowMgr.init(espnowMode, espnowChannel)) {
        Serial.println("[✓ OK  ] ESP-NOW inicializado");

        if (espnowMode == "sensor") {
          // Sensor mode: attempt discovery
          Serial.println("[→ INFO] Modo sensor: buscando gateway...");
          if (espnowMgr.waitForDiscovery()) {
            Serial.println("[✓ OK  ] Gateway encontrado y emparejado");
          } else {
            Serial.println("[⚠ WARN] Gateway no encontrado (reintentará automáticamente)");
          }
        } else {
          // Gateway mode: register mesh data callback and start beacon
          espnowMgr.setMeshDataCallback(onMeshDataReceived);
          Serial.println("[→ INFO] Modo gateway: broadcasting beacon + forwarding mesh data");
        }
      } else {
        Serial.println("[✗ ERR ] Error inicializando ESP-NOW");
      }
    } else {
      Serial.println("[→ INFO] ESP-NOW deshabilitado en configuración");
    }
  #endif

  server.begin();
  Serial.println("[✓ OK  ] Servidor web iniciado en puerto 80");

  Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("  SISTEMA LISTO");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("\n  Punto de Acceso: " + wifiManager.getAPSSID());
  Serial.println("  Configuración:   http://192.168.16.10");
  Serial.println("  Panel Web:       http://<IP>/settings");
  Serial.println("  Datos Sensores:  http://<IP>/data");
  Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");

}

void loop() {
  wifiManager.update();
  static unsigned long lastStatusPrint = 0;
  if (millis() - lastStatusPrint > 30000) {  // Print status every 30 seconds
      lastStatusPrint = millis();

      if (wifiManager.isOnline()) {
          Serial.println("WiFi Status: Connected to " + wifiManager.getCurrentSSID());
          Serial.println("IP Address: " + wifiManager.getLocalIP().toString());
      } else {
          Serial.println("WiFi Status: Disconnected - AP available at " + wifiManager.getAPSSID());
      }
  }
  server.handleClient();

  #ifdef ENABLE_ESPNOW
    // Update ESP-NOW (beacon broadcast for gateway, retry discovery for sensor)
    espnowMgr.update();

    // Process buffered mesh data (gateway only)
    // This runs in main loop context, safe for HTTP calls
    while (meshBufferTail != meshBufferHead) {
      MeshDataBuffer* data = &meshBuffer[meshBufferTail];
      Serial.printf("[MESH→GRAFANA] Processing buffered data from %02X:%02X:%02X:%02X:%02X:%02X (seq=%lu)\n",
                 data->senderMAC[0], data->senderMAC[1], data->senderMAC[2], data->senderMAC[3], data->senderMAC[4], data->senderMAC[5], data->seq);
      if (data->valid) {
        char deviceid[32];
        snprintf(deviceid, sizeof(deviceid), "moni-%02X%02X%02X%02X%02X%02X",
                 data->senderMAC[0], data->senderMAC[1], data->senderMAC[2], data->senderMAC[3], data->senderMAC[4], data->senderMAC[5]);

        Serial.printf("[MESH→GRAFANA] %s: T=%.1f H=%.1f CO2=%.0f (seq=%lu)\n",
                      deviceid, data->temp, data->hum, data->co2, data->seq);

        // Now safe to make HTTP call from main loop
        sendDataGrafana(data->temp, data->hum, data->co2, data->sensorId, deviceid);

        data->valid = false;  // Mark as processed
      }

      // Move to next buffer entry
      meshBufferTail = (meshBufferTail + 1) % MESH_BUFFER_SIZE;
    }
  #endif

  unsigned long currentMillis = millis();

  //// 1. Verificamos si hay que chequear actualizaciones
  if (currentMillis - lastUpdateCheck >= UPDATE_INTERVAL) {
    Serial.printf("Free heap before checking: %d bytes\n", ESP.getFreeHeap());
    checkForUpdates();
    Serial.printf("Free heap after checking: %d bytes\n", ESP.getFreeHeap());
    lastUpdateCheck = currentMillis;
  }

  //// 2. Enviamos datos a Grafana cada 10 segundos
  if (currentMillis - lastSendTime >= 10000) {
    lastSendTime = currentMillis;

    #ifdef SENSOR_MULTI
      // Modo multi-sensor: leer y enviar todos los sensores
      sensorMgr.readAll();

      Serial.printf("Free heap before sending: %d bytes\n", ESP.getFreeHeap());

      for (auto* s : sensorMgr.getSensors()) {
        if (s->isActive()) {
          float temperature = s->getTemperature();
          float humidity = s->getHumidity();
          float co2 = s->getCO2();

          String sensorId = sensorMgr.getSensorId(s);

          Serial.printf("[%s] Temp: %.1f°C, Hum: %.1f%%, CO2: %.0fppm\n",
                       s->getSensorID(), temperature, humidity, co2);

          // Enviar a Grafana
          sendDataGrafana(s->getMeasurementsString(), s->getSensorID());

          #ifdef ENABLE_RS485
            // Enviar por RS485
            rs485.sendSensorData(temperature, humidity, co2, s->getSensorID());
          #endif

          #ifdef ENABLE_ESPNOW
            // Enviar por ESP-NOW (solo si es sensor y está emparejado)
            if (espnowMgr.getMode() == "sensor" && espnowMgr.isPaired()) {
              espnowMgr.sendSensorData(temperature, humidity, co2, s->getSensorID());
            }
          #endif
        }
      }

      Serial.printf("Free heap after sending: %d bytes\n", ESP.getFreeHeap());

    #else
      // Modo single sensor (backward compatible)
      float temperature = 99, humidity = 100, co2 = 999999;

      if (sensor && sensor->isActive() && sensor->dataReady()) {
        if (sensor->read()) {
          temperature = sensor->getTemperature();
          humidity = sensor->getHumidity();
          co2 = sensor->getCO2();

          Serial.printf("[%s] Temp: %.1f°C, Hum: %.1f%%, CO2: %.0fppm\n",
                       sensor->getSensorType(), temperature, humidity, co2);
        } else {
          Serial.println("Error leyendo el sensor!");
          return;
        }
      } else {
        Serial.println("Sensor no listo, esperando...");
      }

      Serial.printf("Free heap before sending: %d bytes\n", ESP.getFreeHeap());
      sendDataGrafana(sensor->getMeasurementsString(), sensor->getSensorID());
      Serial.printf("Free heap after sending: %d bytes\n", ESP.getFreeHeap());

      #ifdef ENABLE_RS485
        // También enviar datos por RS485
        rs485.sendSensorData(temperature, humidity, co2, sensor ? sensor->getSensorType() : "Unknown");
      #endif
    #endif

    // Report Relays to Grafana
    for (auto* r : relayMgr.getRelays()) {
        if (r && r->isActive()) {
            r->syncState();
            r->syncInputs(); 
            
            String data = r->getGrafanaString();
            String id = r->getAlias();
            if (id.length() == 0) id = "relay_" + String(r->getAddress());
            id.replace(" ", "_");
            
            sendDataGrafana(data.c_str(), id.c_str());
        }
    }
  }
  delay(10);
}

#endif  // UNIT_TEST