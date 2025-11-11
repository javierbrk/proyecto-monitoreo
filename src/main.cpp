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

#ifdef SENSOR_MULTI
  #include "SensorManager.h"
  SensorManager sensorMgr;
#else
  #include "sensors/SensorFactory.h"
#endif

#ifdef ENABLE_RS485
  #include "RS485Manager.h"
  RS485Manager rs485;
#endif

unsigned long lastUpdateCheck = 0;
unsigned long lastSendTime = 0;

#ifndef UNIT_TEST

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

  Serial.println("[INFO] Iniciando sistema...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("  INICIALIZACIÓN DEL SISTEMA");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

  if (!SPIFFS.begin(true)) {
    Serial.println("[ERROR] ✗ No se pudo montar SPIFFS");
  } else {
    Serial.println("[OK]    ✓ SPIFFS montado correctamente");
  }

  createConfigFile();

  Serial.println("\n[INFO] Inicializando sensores...");
  #ifdef SENSOR_MULTI
    JsonDocument configDoc = loadConfig();
    sensorMgr.loadFromConfig(configDoc);
    int sensorCount = sensorMgr.getSensorCount();
    Serial.printf("[OK]    ✓ Modo multi-sensor: %d sensor%s activo%s\n",
                  sensorCount, sensorCount != 1 ? "es" : "", sensorCount != 1 ? "s" : "");

    // Listar sensores activos
    for (auto* s : sensorMgr.getSensors()) {
      if (s && s->isActive()) {
        String sensorId = sensorMgr.getSensorId(s);
        Serial.printf("        ├─ %s\n", sensorId.c_str());
      }
    }
  #else
    sensor = SensorFactory::createSensor();
    if (sensor) {
      if (sensor->init()) {
        Serial.printf("[OK]    ✓ Sensor %s inicializado\n", sensor->getSensorType());
      } else {
        Serial.printf("[ERROR] ✗ Error inicializando %s\n", sensor->getSensorType());
      }
    } else {
      Serial.println("[ERROR] ✗ No se pudo crear el sensor");
    }
  #endif

  #ifdef ENABLE_RS485
    Serial.println("\n[INFO] Configurando RS485...");
    rs485.init(16, 17, 9600);
    Serial.println("[OK]    ✓ RS485 habilitado (TX: GPIO17, RX: GPIO16, 9600 baud)");
  #endif

  clientSecure.setInsecure(); 

  server.on("/actual", HTTP_GET, handleMediciones);
  server.on("/config", HTTP_GET, handleConfiguracion);
  server.on("/config", HTTP_POST, habldePostConfig);
  server.on("/data", HTTP_GET, handleData);
  server.on("/calibrate-scd30", HTTP_GET, handleSCD30Calibration);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/restart", HTTP_POST, handleRestart);
    // Add handler for undefined routes
  server.onNotFound([]() {
      // Redirect all undefined routes to root page
      Serial.println("Redirecting to root page for undefined route");
      Serial.printf("Requested route: %s\n", server.uri().c_str());
      server.sendHeader("Location", "/", true);
      server.send(302, "text/plain", "");
  });



  server.enableCORS(true);

  Serial.println("\n[INFO] Configurando WiFi Manager...");
  wifiManager.setConnectionTimeout(15000);
  wifiManager.setMaxRetries(8);
  wifiManager.setValidationTimeout(30000);
  wifiManager.init(&server);
  Serial.println("[OK]    ✓ WiFi Manager inicializado");

  server.begin();
  Serial.println("[OK]    ✓ Servidor web iniciado en puerto 80");

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
                       sensorId.c_str(), temperature, humidity, co2);

          // Enviar a Grafana
          sendDataGrafana(temperature, humidity, co2, sensorId.c_str());

          #ifdef ENABLE_RS485
            // Enviar por RS485
            rs485.sendSensorData(temperature, humidity, co2, sensorId.c_str());
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
      sendDataGrafana(temperature, humidity, co2, sensor ? sensor->getSensorType() : "Unknown");
      Serial.printf("Free heap after sending: %d bytes\n", ESP.getFreeHeap());

      #ifdef ENABLE_RS485
        // También enviar datos por RS485
        rs485.sendSensorData(temperature, humidity, co2, sensor ? sensor->getSensorType() : "Unknown");
      #endif
    #endif
  }
  delay(10);
}

#endif  // UNIT_TEST