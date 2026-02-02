#include "actuators/RelayManager.h"
#include "configFile.h"
#include "constants.h"
#include "createGrafanaMessage.h"
#include "debug.h"
#include "endpoints.h"
#include "globals.h"
#include "otaUpdater.h"
#include "sendDataGrafana.h"
#include "version.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <esp_ota_ops.h>
#include <time.h>

#ifdef ENABLE_OTA
#include <ArduinoOTA.h>
#endif

#ifdef SENSOR_MULTI
#include "SensorManager.h"
#include "sensors/ICO2Sensor.h"
#include "sensors/IHumiditySensor.h"
#include "sensors/IMoistureSensor.h"
#include "sensors/ITemperatureSensor.h"
SensorManager sensorMgr;
// Wrapper function for endpoints.cpp to access sensors without including full
// header
std::vector<ISensor *> &getSensorList() { return sensorMgr.getSensors(); }
#else
#include "sensors/ICO2Sensor.h"
#include "sensors/IHumiditySensor.h"
#include "sensors/ITemperatureSensor.h"
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
  char sensorId[32];
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
// IMPORTANT: Runs in WiFi interrupt context - must not call HTTP/blocking
// functions
void onMeshDataReceived(const uint8_t *senderMAC, float temp, float hum,
                        float co2, uint32_t seq, const char *sensorId) {
  // Calculate next buffer position
  int nextHead = (meshBufferHead + 1) % MESH_BUFFER_SIZE;

  // Check if buffer is full
  if (nextHead == meshBufferTail) {
    DBG_ERROR("[MESH] Buffer full, dropping data\n");
    return;
  }

  // Store data in buffer
  memcpy(meshBuffer[meshBufferHead].senderMAC, senderMAC, 6);

  if (sensorId != nullptr) {
    strncpy(meshBuffer[meshBufferHead].sensorId, sensorId,
            sizeof(meshBuffer[meshBufferHead].sensorId) - 1);
    meshBuffer[meshBufferHead]
        .sensorId[sizeof(meshBuffer[meshBufferHead].sensorId) - 1] = '\0';
  } else {
    strcpy(meshBuffer[meshBufferHead].sensorId, "unknown");
  }
  meshBuffer[meshBufferHead].temp = temp;
  meshBuffer[meshBufferHead].hum = hum;
  meshBuffer[meshBufferHead].co2 = co2;
  meshBuffer[meshBufferHead].seq = seq;
  meshBuffer[meshBufferHead].valid = true;

  // Update head pointer (atomic for single-writer scenario)
  meshBufferHead = nextHead;
}

String detectRole(const JsonDocument &config) {
  DBG_INFOLN("\n[INFO] Auto-detecting device role...");

  // 1. Check if WiFi is connected
  if (!wifiManager.isOnline()) {
    DBG_INFOLN("  No WiFi -> SENSOR mode");
    return "sensor";
  }

  DBG_INFOLN("  WiFi connected, checking Grafana...");

  // 2. Test Grafana connectivity
  String grafanaUrl = config["grafana_ping_url"] | "http://192.168.1.1/ping";

  HTTPClient http;
  http.begin(grafanaUrl);
  http.setTimeout(3000); // 3 second timeout

  int httpCode = http.GET();
  http.end();

  if (httpCode > 0) {
    DBG_INFO("  Grafana OK (HTTP %d) -> GATEWAY mode\n", httpCode);
    return "gateway";
  } else {
    DBG_INFO("  Grafana unreachable (%d) -> SENSOR mode\n", httpCode);
    return "sensor";
  }
}
#endif

void printBanner() {
  DBG_INFOLN("\n  ALTERMUNDI - Proyecto Monitoreo");
  DBG_INFOLN("  La pata tecnologica de ese otro mundo posible\n");
}

void setup() {
  DEBUG_BEGIN(115200);
  delay(500);

  IF_INFO(printBanner());

  DBG_INFOLN("[INFO] Starting system...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  DBG_INFOLN("=== SYSTEM INIT ===");

  if (!SPIFFS.begin(true)) {
    DBG_ERRORLN("[ERR] SPIFFS mount failed");
  } else {
    DBG_INFOLN("[OK] SPIFFS mounted");
  }

  createConfigFile();

  // Load configuration ONCE for all modules
  JsonDocument config = loadConfig();

  IF_VERBOSE({
    DBG_INFOLN("\n[INFO] Config loaded:");
    String configOutput;
    serializeJsonPretty(config, configOutput);
    DBG_INFOLN(configOutput.c_str());
  });

#ifdef ENABLE_RS485
  DBG_INFOLN("\n[INFO] Configuring RS485...");
  JsonObject rs485Cfg = config["rs485"];
  bool rs485Enabled = rs485Cfg["enabled"] | false;

  if (!rs485Enabled) {
    DBG_INFOLN("[INFO] RS485/Modbus disabled in config");
  } else {
    int rx = rs485Cfg["rx_pin"] | 16;
    int tx = rs485Cfg["tx_pin"] | 17;
    int de = rs485Cfg["de_pin"] | 18;
    int baud = rs485Cfg["baudrate"] | 9600;
    bool rawSendEnabled = rs485Cfg["raw_send_enabled"] | false;

    DBG_INFO("[INFO] RS485: RX=%d TX=%d DE=%d Baud=%d\n", rx, tx, de, baud);
    DBG_VERBOSE("[INFO] RS485 Raw Send: %s\n",
                rawSendEnabled ? "enabled" : "disabled");

    // 1. Initialize ModbusManager first (Singleton owner of the bus)
    ModbusManager::getInstance().begin(rx, tx, de, baud);

    // 2. Initialize RS485Manager (will reuse ModbusManager's serial if
    // available)
    rs485.init(rx, tx, baud, de, de);
    rs485.setRawSendEnabled(rawSendEnabled);

    DBG_INFOLN("[OK] RS485/Modbus enabled");
    delay(100);
  }
#endif

  DBG_INFOLN("\n[INFO] Initializing sensors...");
#ifdef SENSOR_MULTI
  sensorMgr.loadFromConfig(config);

  // Configure delay between sensor readings (helps prevent RS485/Modbus
  // collisions)
  uint16_t modbusDelay = config["modbus_delay_ms"] | 50;
  sensorMgr.setModbusDelay(modbusDelay);

  int sensorCount = sensorMgr.getSensorCount();
  DBG_INFO("[OK] Multi-sensor: %d active\n", sensorCount);

  IF_VERBOSE({
    for (auto *s : sensorMgr.getSensors()) {
      if (s && s->isActive()) {
        DBG_VERBOSE("  - %s\n", sensorMgr.getSensorId(s).c_str());
      }
    }
  });

  // Initialize Relays
  DBG_INFOLN("\n[INFO] Initializing Relays...");
  relayMgr.loadFromConfig(config);
  DBG_INFO("[OK] %d relays configured\n", relayMgr.getRelays().size());
  for (auto *r : relayMgr.getRelays()) {
    if (r)
      r->init();
  }
#else
  sensor = SensorFactory::createSensor();
  if (sensor) {
    if (sensor->init()) {
      DBG_INFO("[OK] Sensor %s initialized\n", sensor->getSensorType());
    } else {
      DBG_ERROR("[ERR] Init failed: %s\n", sensor->getSensorType());
    }
  } else {
    DBG_ERRORLN("[ERR] Could not create sensor");
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
    DBG_VERBOSE("404 redirect: %s\n", server.uri().c_str());
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  });

  server.enableCORS(true);

  DBG_INFOLN("\n[INFO] Configuring WiFi Manager...");
  wifiManager.setConnectionTimeout(15000);
  wifiManager.setMaxRetries(8);
  wifiManager.setValidationTimeout(30000);
  wifiManager.init(&server);
  DBG_INFOLN("[OK] WiFi Manager initialized");

#ifdef ENABLE_ESPNOW
  DBG_INFOLN("\n[INFO] Configuring ESP-NOW...");
  bool espnowEnabled = config["espnow_enabled"] | false;

  if (espnowEnabled) {
    // Auto-detect role or use forced mode
    String forcedMode = config["espnow_force_mode"] | "";
    String espnowMode;

    if (forcedMode != "") {
      DBG_INFO("[INFO] Forced mode: %s\n", forcedMode.c_str());
      espnowMode = forcedMode;
    } else {
      espnowMode = detectRole(config);
    }

    uint8_t espnowChannel = config["espnow_channel"] | 1;

    if (espnowChannel < 1 || espnowChannel > 13) {
      DBG_INFO("[WARN] Invalid channel %d, using 1\n", espnowChannel);
      espnowChannel = 1;
    }

    DBG_INFO("[INFO] ESP-NOW mode: %s (ch %d)\n", espnowMode.c_str(),
             espnowChannel);

    if (espnowMode == "gateway") {
      if (wifiManager.isOnline()) {
        uint8_t wifiChannel = WiFi.channel();
        if (wifiChannel >= 1 && wifiChannel <= 13) {
          espnowChannel = wifiChannel;
          DBG_VERBOSE("[INFO] Gateway using WiFi channel: %d\n", espnowChannel);
        }
      }
    }

    if (espnowMgr.init(espnowMode, espnowChannel)) {
      DBG_INFOLN("[OK] ESP-NOW initialized");

      if (espnowMode == "sensor") {
        DBG_INFOLN("[INFO] Sensor mode: searching gateway...");
        if (espnowMgr.waitForDiscovery()) {
          DBG_INFOLN("[OK] Gateway found and paired");
        } else {
          DBG_INFOLN("[WARN] Gateway not found (will retry)");
        }
      } else {
        espnowMgr.setMeshDataCallback(onMeshDataReceived);
        DBG_INFOLN("[INFO] Gateway mode: beacon + forwarding");
      }
    } else {
      DBG_ERRORLN("[ERR] ESP-NOW init failed");
    }
  } else {
    DBG_INFOLN("[INFO] ESP-NOW disabled in config");
  }
#endif

  server.begin();
  DBG_INFOLN("[OK] Web server started on port 80");

#ifdef ENABLE_OTA
  DBG_INFOLN("\n[INFO] Configuring OTA...");
  ArduinoOTA.setHostname(wifiManager.getAPSSID().c_str());
  ArduinoOTA.onStart([]() {
    String type =
        (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    DBG_INFO("[OTA] Start updating %s\n", type.c_str());
  });
  ArduinoOTA.onEnd([]() { DBG_INFOLN("\n[OTA] End"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DBG_VERBOSE("[OTA] Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    DBG_ERROR("[OTA] Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      DBG_ERRORLN("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      DBG_ERRORLN("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      DBG_ERRORLN("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      DBG_ERRORLN("Receive Failed");
    else if (error == OTA_END_ERROR)
      DBG_ERRORLN("End Failed");
  });
  ArduinoOTA.begin();
  DBG_INFO("[OK] OTA ready (port 3232)\n");
#endif

  DBG_INFOLN("\n=== SYSTEM READY ===");
  DBG_INFO("  AP: %s\n", wifiManager.getAPSSID().c_str());
  DBG_INFOLN("  Config: http://192.168.16.10");
  DBG_INFOLN("  Data:   http://<IP>/data\n");
}

void loop() {
#ifdef ENABLE_OTA
  ArduinoOTA.handle();
#endif
  wifiManager.update();

  IF_VERBOSE({
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint > 30000) {
      lastStatusPrint = millis();
      if (wifiManager.isOnline()) {
        DBG_VERBOSE("WiFi: %s IP: %s\n", wifiManager.getCurrentSSID().c_str(),
                    wifiManager.getLocalIP().toString().c_str());
      } else {
        DBG_VERBOSE("WiFi: Disconnected, AP: %s\n",
                    wifiManager.getAPSSID().c_str());
      }
    }
  });

  server.handleClient();

#ifdef ENABLE_ESPNOW
  espnowMgr.update();

  // Process buffered mesh data (gateway only)
  while (meshBufferTail != meshBufferHead) {
    MeshDataBuffer *data = &meshBuffer[meshBufferTail];
    if (data->valid) {
      char deviceid[32];
      snprintf(deviceid, sizeof(deviceid), "moni-%02X%02X%02X%02X%02X%02X",
               data->senderMAC[0], data->senderMAC[1], data->senderMAC[2],
               data->senderMAC[3], data->senderMAC[4], data->senderMAC[5]);

      DBG_VERBOSE("[MESH] %s: T=%.1f H=%.1f CO2=%.0f\n", deviceid, data->temp,
                  data->hum, data->co2);

      sendDataGrafana(data->temp, data->hum, data->co2, data->sensorId,
                      deviceid);
      data->valid = false;
    }
    meshBufferTail = (meshBufferTail + 1) % MESH_BUFFER_SIZE;
  }
#endif

  unsigned long currentMillis = millis();

  // Check for updates
  if (currentMillis - lastUpdateCheck >= UPDATE_INTERVAL) {
    DBG_VERBOSE("Free heap: %d bytes\n", ESP.getFreeHeap());
    checkForUpdates();
    lastUpdateCheck = currentMillis;
  }

  // Send data to Grafana every 10 seconds
  if (currentMillis - lastSendTime >= 10000) {
    lastSendTime = currentMillis;

#ifdef SENSOR_MULTI
    sensorMgr.readAll();

    for (auto *s : sensorMgr.getSensors()) {
      if (s->isActive()) {
        float temperature = -999, humidity = -999, co2 = -999;

        auto *tempSensor = dynamic_cast<ITemperatureSensor *>(s);
        auto *humSensor = dynamic_cast<IHumiditySensor *>(s);
        auto *moistSensor = dynamic_cast<IMoistureSensor *>(s);
        auto *co2Sensor = dynamic_cast<ICO2Sensor *>(s);

        if (tempSensor)
          temperature = tempSensor->getTemperature();
        if (humSensor)
          humidity = humSensor->getHumidity();
        if (moistSensor)
          humidity = moistSensor->getMoisture();
        if (co2Sensor)
          co2 = co2Sensor->getCO2();

        DBG_INFO("[%s] %s\n", s->getSensorID(), s->getMeasurementsString());

        sendDataGrafana(s->getMeasurementsString(), s->getSensorID());

#ifdef ENABLE_RS485
        rs485.sendSensorData(temperature, humidity, co2, s->getSensorID());
#endif

#ifdef ENABLE_ESPNOW
        if (espnowMgr.getMode() == "sensor" && espnowMgr.isPaired()) {
          espnowMgr.sendSensorData(temperature, humidity, co2,
                                   s->getSensorID());
        }
#endif
      }
    }

#else
    float temperature = 99, humidity = 100, co2 = 999999;

    if (sensor && sensor->isActive() && sensor->dataReady()) {
      if (sensor->read()) {
        auto *tempSensor = dynamic_cast<ITemperatureSensor *>(sensor);
        auto *humSensor = dynamic_cast<IHumiditySensor *>(sensor);
        auto *co2Sensor = dynamic_cast<ICO2Sensor *>(sensor);

        if (tempSensor)
          temperature = tempSensor->getTemperature();
        if (humSensor)
          humidity = humSensor->getHumidity();
        if (co2Sensor)
          co2 = co2Sensor->getCO2();

        DBG_INFO("[%s] %s\n", sensor->getSensorType(),
                 sensor->getMeasurementsString());
      } else {
        DBG_ERRORLN("Sensor read error!");
        return;
      }
    } else {
      DBG_VERBOSELN("Sensor not ready...");
    }

    sendDataGrafana(sensor->getMeasurementsString(), sensor->getSensorID());

#ifdef ENABLE_RS485
    rs485.sendSensorData(temperature, humidity, co2,
                         sensor ? sensor->getSensorType() : "Unknown");
#endif
#endif

    // Report Relays to Grafana
    for (auto *r : relayMgr.getRelays()) {
      if (r && r->isActive()) {
        r->syncState();
        r->syncInputs();

        String data = r->getGrafanaString();
        String id = r->getAlias();
        if (id.length() == 0)
          id = "relay_" + String(r->getAddress());
        id.replace(" ", "_");

        sendDataGrafana(data.c_str(), id.c_str());
      }
    }
  }
  delay(10);
}

#endif // UNIT_TEST