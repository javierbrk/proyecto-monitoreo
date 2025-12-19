#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include "endpoints.h"
#include "globals.h"
#include "constants.h"
#include "configFile.h"
#include "webConfigPage.h"
#include "actuators/RelayManager.h"
#include "debug.h"

#include <ArduinoJson.h>

#ifdef SENSOR_MULTI
  #include <vector>
  #include "sensors/ISensor.h"
  #include "sensors/ITemperatureSensor.h"
  #include "sensors/IHumiditySensor.h"
  #include "sensors/IMoistureSensor.h"
  #include "sensors/ICO2Sensor.h"
  #include "sensors/IPressureSensor.h"
  #include "sensors/ISoilSensor.h"
  class SensorManager;
  extern SensorManager sensorMgr;
  extern std::vector<ISensor*>& getSensorList();
#endif

#ifdef ENABLE_ESPNOW
  #include "ESPNowManager.h"
  extern ESPNowManager espnowMgr;
#endif

extern RelayManager relayMgr;

void handleMediciones() {
    float temperature = 99, humidity = 100, co2 = 999999, presion = 99;
    String wifiStatus = "unknown";
    bool rotation = false;

#ifdef SENSOR_MULTI
    // Use first sensor that has temperature/humidity/co2
    for (auto* s : getSensorList()) {
        if (!s || !s->isActive() || !s->dataReady()) continue;
        s->read();

        auto* tempSensor = dynamic_cast<ITemperatureSensor*>(s);
        auto* humSensor = dynamic_cast<IHumiditySensor*>(s);
        auto* co2Sensor = dynamic_cast<ICO2Sensor*>(s);

        if (tempSensor) temperature = tempSensor->getTemperature();
        if (humSensor) humidity = humSensor->getHumidity();
        if (co2Sensor) co2 = co2Sensor->getCO2();

        wifiStatus = (WiFi.status() == WL_CONNECTED) ? "connected" : "disconnected";
        break;  // Use first valid sensor
    }
#else
    if (sensor && sensor->isActive() && sensor->dataReady() && sensor->read()) {
        auto* tempSensor = dynamic_cast<ITemperatureSensor*>(sensor);
        auto* humSensor = dynamic_cast<IHumiditySensor*>(sensor);
        auto* co2Sensor = dynamic_cast<ICO2Sensor*>(sensor);

        if (tempSensor) temperature = tempSensor->getTemperature();
        if (humSensor) humidity = humSensor->getHumidity();
        if (co2Sensor) co2 = co2Sensor->getCO2();

        wifiStatus = (WiFi.status() == WL_CONNECTED) ? "connected" : "disconnected";
    }
#endif

    JsonDocument doc;
    doc["rotation"] = rotation;
    doc["a_pressure"] = String(presion, 2);

    JsonObject errors = doc["errors"].to<JsonObject>();
    errors["rotation"].to<JsonArray>();
    errors["temperature"].to<JsonArray>();
    errors["sensors"].to<JsonArray>();
    errors["humidity"].to<JsonArray>();
    errors["wifi"].to<JsonArray>();

    doc["a_temperature"] = String(temperature, 2);
    doc["a_humidity"] = String(humidity, 2);
    doc["a_co2"] = String(co2, 2);
    doc["wifi_status"] = wifiStatus;

    String output;
    serializeJsonPretty(doc, output);
    server.send(200, "application/json", output);
}

// Helper to get sensor icon based on capabilities
String getSensorIcon(ISensor* s) {
#ifdef SENSOR_MULTI
    if (dynamic_cast<ICO2Sensor*>(s)) return "🌬️";
    if (dynamic_cast<ISoilSensor*>(s)) return "🌱";
    if (dynamic_cast<IMoistureSensor*>(s)) return "🌱";
    if (dynamic_cast<IPressureSensor*>(s)) return "🌡️";
    if (dynamic_cast<ITemperatureSensor*>(s)) return "🌡️";
#endif
    return "📊";
}

void handleData() {
    String wifiStatus = (WiFi.status() == WL_CONNECTED) ? "Conectado" : "Desconectado";
    int wifiRSSI = WiFi.RSSI();

    // HTML header with minimal CSS
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<meta http-equiv='refresh' content='10'>";
    html += "<title>Datos - Monitor</title>";
    html += "<style>";
    html += ":root{--g:#55d400;--o:#F39100;--r:#dc3545}";
    html += "*{margin:0;padding:0;box-sizing:border-box}";
    html += "body{font-family:system-ui,-apple-system,sans-serif;background:#f5f5f5;padding:15px;min-height:100vh}";
    html += "h1{color:#333;text-align:center;margin-bottom:15px;font-size:1.4em}";
    html += ".cards{display:flex;flex-wrap:wrap;gap:12px;justify-content:center}";
    html += ".card{background:#fff;border-radius:8px;padding:15px;min-width:280px;max-width:350px;flex:1;";
    html += "box-shadow:0 2px 4px rgba(0,0,0,.1);border-left:4px solid var(--g)}";
    html += ".card.err{border-left-color:var(--r);opacity:.7}";
    html += ".card.warn{border-left-color:var(--o)}";
    html += ".hdr{display:flex;justify-content:space-between;align-items:center;margin-bottom:10px}";
    html += ".type{font-weight:600;color:#333;font-size:1.1em}";
    html += ".id{font-size:.7em;color:#888;background:#f0f0f0;padding:2px 6px;border-radius:3px}";
    html += ".vals{display:grid;grid-template-columns:1fr 1fr;gap:8px}";
    html += ".val{padding:10px 8px;background:#f9f9f9;border-radius:6px;text-align:center}";
    html += ".val span{display:block;font-size:.7em;color:#666;margin-bottom:2px}";
    html += ".val b{font-size:1.3em;color:#333}";
    html += ".val.ok b{color:var(--g)}.val.warn b{color:var(--o)}.val.bad b{color:var(--r)}";
    html += ".status{text-align:center;margin-top:15px;padding:10px;background:#fff;border-radius:6px;";
    html += "font-size:.85em;color:#666;box-shadow:0 1px 3px rgba(0,0,0,.08)}";
    html += ".status b{color:#333}";
    html += ".empty{text-align:center;padding:40px;color:#888}";
    html += "</style><script>function toggle(a,c){fetch('/api/relay/toggle?addr='+a+'&ch='+c,{method:'POST'}).then(r=>{if(r.ok)location.reload()})}</script>";
    html += "</head><body>";
    html += "<h1>📊 Datos de Sensores</h1>";
    html += "<div class='cards'>";

    int sensorCount = 0;

#ifdef SENSOR_MULTI
    for (auto* s : getSensorList()) {
        if (!s) continue;
        sensorCount++;

        bool isActive = s->isActive();
        bool hasData = isActive && s->dataReady();
        if (hasData) s->read();

        // Check capabilities via interfaces
        auto* tempSensor = dynamic_cast<ITemperatureSensor*>(s);
        auto* humSensor = dynamic_cast<IHumiditySensor*>(s);
        auto* moistSensor = dynamic_cast<IMoistureSensor*>(s);
        auto* co2Sensor = dynamic_cast<ICO2Sensor*>(s);
        auto* pressSensor = dynamic_cast<IPressureSensor*>(s);
        auto* soilSensor = dynamic_cast<ISoilSensor*>(s);

        bool hasError = !isActive || !hasData;
        String cardClass = hasError ? " err" : "";

        html += "<div class='card" + cardClass + "'>";
        html += "<div class='hdr'>";
        html += "<span class='type'>" + getSensorIcon(s) + " " + String(s->getSensorType()) + "</span>";
        html += "<span class='id'>" + String(s->getSensorID()) + "</span>";
        html += "</div>";
        html += "<div class='vals'>";

        bool hasAnyReading = false;

        // Temperature
        if (tempSensor && hasData) {
            float temp = tempSensor->getTemperature();
            if (temp > -100 && temp < 100) {
                String cls = (temp < 10 || temp > 35) ? " warn" : " ok";
                html += "<div class='val" + cls + "'><span>🌡️ Temperatura</span><b>" + String(temp, 1) + "°C</b></div>";
                hasAnyReading = true;
            }
        }

        // Air Humidity
        if (humSensor && hasData) {
            float hum = humSensor->getHumidity();
            if (hum >= 0 && hum <= 100) {
                String cls = (hum < 30 || hum > 80) ? " warn" : " ok";
                html += "<div class='val" + cls + "'><span>💧 Humedad</span><b>" + String(hum, 1) + "%</b></div>";
                hasAnyReading = true;
            }
        }

        // Soil Moisture
        if (moistSensor && hasData) {
            float moist = moistSensor->getMoisture();
            if (moist >= 0 && moist <= 100) {
                String cls = (moist < 30) ? " warn" : " ok";
                html += "<div class='val" + cls + "'><span>🌱 Humedad suelo</span><b>" + String(moist, 1) + "%</b></div>";
                hasAnyReading = true;
            }
        }

        // CO2
        if (co2Sensor && hasData) {
            float co2 = co2Sensor->getCO2();
            if (co2 > 0 && co2 < 10000) {
                String cls = co2 > 1000 ? " bad" : (co2 > 800 ? " warn" : " ok");
                html += "<div class='val" + cls + "'><span>🌬️ CO₂</span><b>" + String((int)co2) + " ppm</b></div>";
                hasAnyReading = true;
            }
        }

        // Pressure
        if (pressSensor && hasData) {
            float press = pressSensor->getPressure();
            if (press > 0) {
                html += "<div class='val ok'><span>🔵 Presión</span><b>" + String(press, 1) + " hPa</b></div>";
                hasAnyReading = true;
            }
        }

        // Soil sensor extended data (EC, pH, NPK)
        if (soilSensor && hasData) {
            float ec = soilSensor->getEC();
            float ph = soilSensor->getPH();
            int nitrogen = soilSensor->getNitrogen();
            int phosphorus = soilSensor->getPhosphorus();
            int potassium = soilSensor->getPotassium();

            if (ec >= 0) {
                html += "<div class='val ok'><span>⚡ EC</span><b>" + String((int)ec) + " μS/cm</b></div>";
                hasAnyReading = true;
            }
            if (ph >= 0) {
                String cls = (ph < 5.5 || ph > 7.5) ? " warn" : " ok";
                html += "<div class='val" + cls + "'><span>🧪 pH</span><b>" + String(ph, 1) + "</b></div>";
                hasAnyReading = true;
            }
            if (nitrogen >= 0) {
                html += "<div class='val ok'><span>🌿 N</span><b>" + String(nitrogen) + " mg/kg</b></div>";
                hasAnyReading = true;
            }
            if (phosphorus >= 0) {
                html += "<div class='val ok'><span>🔷 P</span><b>" + String(phosphorus) + " mg/kg</b></div>";
                hasAnyReading = true;
            }
            if (potassium >= 0) {
                html += "<div class='val ok'><span>🟡 K</span><b>" + String(potassium) + " mg/kg</b></div>";
                hasAnyReading = true;
            }
        }

        // If no valid readings
        if (!hasAnyReading) {
            html += "<div class='val'><span>Estado</span><b>" + String(isActive ? "Sin datos" : "Inactivo") + "</b></div>";
        }

        html += "</div></div>";
    }
#else
    // Single sensor mode
    if (sensor) {
        sensorCount = 1;
        bool isActive = sensor->isActive();
        bool hasData = isActive && sensor->dataReady();
        if (hasData) sensor->read();

        auto* tempSensor = dynamic_cast<ITemperatureSensor*>(sensor);
        auto* humSensor = dynamic_cast<IHumiditySensor*>(sensor);
        auto* moistSensor = dynamic_cast<IMoistureSensor*>(sensor);
        auto* co2Sensor = dynamic_cast<ICO2Sensor*>(sensor);
        auto* pressSensor = dynamic_cast<IPressureSensor*>(sensor);

        bool hasError = !isActive || !hasData;
        String cardClass = hasError ? " err" : "";

        html += "<div class='card" + cardClass + "'>";
        html += "<div class='hdr'>";
        html += "<span class='type'>📊 " + String(sensor->getSensorType()) + "</span>";
        html += "<span class='id'>" + String(sensor->getSensorID()) + "</span>";
        html += "</div>";
        html += "<div class='vals'>";

        bool hasAnyReading = false;

        if (tempSensor && hasData) {
            float temp = tempSensor->getTemperature();
            if (temp > -100 && temp < 100) {
                String cls = (temp < 10 || temp > 35) ? " warn" : " ok";
                html += "<div class='val" + cls + "'><span>🌡️ Temperatura</span><b>" + String(temp, 1) + "°C</b></div>";
                hasAnyReading = true;
            }
        }

        if (humSensor && hasData) {
            float hum = humSensor->getHumidity();
            if (hum >= 0 && hum <= 100) {
                String cls = (hum < 30 || hum > 80) ? " warn" : " ok";
                html += "<div class='val" + cls + "'><span>💧 Humedad</span><b>" + String(hum, 1) + "%</b></div>";
                hasAnyReading = true;
            }
        }

        if (moistSensor && hasData) {
            float moist = moistSensor->getMoisture();
            if (moist >= 0 && moist <= 100) {
                String cls = (moist < 30) ? " warn" : " ok";
                html += "<div class='val" + cls + "'><span>🌱 Humedad suelo</span><b>" + String(moist, 1) + "%</b></div>";
                hasAnyReading = true;
            }
        }

        if (co2Sensor && hasData) {
            float co2 = co2Sensor->getCO2();
            if (co2 > 0 && co2 < 10000) {
                String cls = co2 > 1000 ? " bad" : (co2 > 800 ? " warn" : " ok");
                html += "<div class='val" + cls + "'><span>🌬️ CO₂</span><b>" + String((int)co2) + " ppm</b></div>";
                hasAnyReading = true;
            }
        }

        if (pressSensor && hasData) {
            float press = pressSensor->getPressure();
            if (press > 0) {
                html += "<div class='val ok'><span>🔵 Presión</span><b>" + String(press, 1) + " hPa</b></div>";
                hasAnyReading = true;
            }
        }

        if (!hasAnyReading) {
            html += "<div class='val'><span>Estado</span><b>" + String(isActive ? "Sin datos" : "Inactivo") + "</b></div>";
        }

        html += "</div></div>";
    }
#endif

    html += "</div>";

    // Relay Section
    if (!relayMgr.getRelays().empty()) {
        html += "<h1 style='margin-top:25px'>🔌 Relés / Actuadores</h1>";
        html += "<div class='cards'>";

        for (auto* r : relayMgr.getRelays()) {
            if (!r) continue;

            bool isActive = r->isActive();
            if (isActive) {
                r->syncState();
                r->syncInputs();
            }

            String cardClass = isActive ? "" : " err";
            html += "<div class='card" + cardClass + "' style='border-left-color:#0198fe'>";
            html += "<div class='hdr'>";
            html += "<span class='type'>Relé Modbus</span>";
            html += "<span class='id'>Addr: " + String(r->getAddress()) + "</span>";
            html += "</div>";

            html += "<div class='vals'>";
            if (r->getAlias().length() > 0) {
                 html += "<div class='val' style='grid-column:span 2;background:none;text-align:left;padding:0 5px'><span>" + r->getAlias() + "</span></div>";
            }

            if (isActive) {
                for(int i=0; i<2; i++) {
                    bool state = r->getState(i);
                    String cls = state ? "ok" : "warn";
                    String label = state ? "ON" : "OFF";
                    html += "<div class='val " + cls + "' onclick='toggle(" + String(r->getAddress()) + "," + String(i) + ")' style='cursor:pointer'>";
                    html += "<span>Canal " + String(i+1) + "</span><b>" + label + "</b></div>";
                }

                for(int i=0; i<2; i++) {
                    bool state = r->getInputState(i);
                    String cls = state ? "ok" : "warn";
                    String label = state ? "ON" : "OFF";
                    html += "<div class='val " + cls + "'><span>Input " + String(i+1) + "</span><b>" + label + "</b></div>";
                }
            } else {
                html += "<div class='val' style='grid-column:span 2;'><span>Estado</span><b>Inactivo</b></div>";
            }
            html += "</div></div>";
        }
        html += "</div>";
    }

    if (sensorCount == 0) {
        html += "<div class='empty'>No hay sensores configurados</div>";
    }

    // Status bar
    html += "<div class='status'>";
    html += "<b>WiFi:</b> " + wifiStatus;
    if (WiFi.status() == WL_CONNECTED) {
        html += " (" + String(wifiRSSI) + " dBm)";
    }
    html += " &nbsp;|&nbsp; <b>Sensores:</b> " + String(sensorCount);
    html += " &nbsp;|&nbsp; <b>Uptime:</b> " + String(millis() / 1000) + "s";
    html += "</div>";

    html += "</body></html>";
    server.send(200, "text/html", html);
}

void handleConfiguracion() {
    JsonDocument doc = loadConfig();

    if (doc.isNull() || doc.size() == 0) {
        server.send(500, "application/json", "{\"error\": \"No se pudo cargar config.json\"}");
        return;
    }

    if (WiFi.status() == WL_CONNECTED) {
        doc["current_wifi_channel"] = WiFi.channel();
    } else {
        doc["current_wifi_channel"] = 0;
    }

    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
}

void habldePostConfig() {
    DBG_INFO("Updating config...\n");

    if (!server.hasArg("plain")) {
      server.send(400, "text/plain", "No JSON data received");
      return;
    }

    String jsonString = server.arg("plain");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonString);

    if (error) {
      DBG_ERROR("JSON parse error: %s\n", error.c_str());
      server.send(400, "text/plain", "Invalid JSON format");
      return;
    }

    const char* new_ssid = doc["ssid"];
    const char* new_password = doc["passwd"];

    if (new_ssid && strlen(new_ssid) > 0 && strcmp(new_ssid, "ToChange") != 0) {
      wifiManager.onChange(String(new_ssid), String(new_password));
      DBG_INFO("WiFi updated: %s\n", new_ssid);
    }

    if (updateConfig(doc)) {
      server.send(200, "text/plain", "Configuration updated successfully. Some changes require restart.");
      DBG_INFO("Config saved\n");
    } else {
      server.send(500, "text/plain", "Failed to save configuration");
      DBG_ERROR("Config save failed\n");
    }
}

void handleSCD30Calibration() {
  DBG_VERBOSE("Calibration called: %s\n", sensor ? sensor->getSensorType() : "NULL");

  String response = "{";
  int httpStatus = 200;

  if (!sensor || !sensor->isActive()) {
      response += "\"status\":\"error\",";
      response += "\"message\":\"No sensor active\",";
      response += "\"sensor_detected\":false,";
      response += "\"calibration_performed\":false";
      httpStatus = 503;
  } else {
      bool calibrationSuccess = sensor->calibrate(400);

      if (calibrationSuccess) {
          response += "\"status\":\"success\",";
          response += "\"message\":\"Sensor calibration completed successfully\",";
          response += "\"sensor_type\":\"" + String(sensor->getSensorType()) + "\",";
          response += "\"sensor_detected\":true,";
          response += "\"calibration_performed\":true,";
          response += "\"target_co2\":400,";
          response += "\"note\":\"Allow 2-3 minutes for sensor to stabilize after calibration\"";
          DBG_INFO("Calibration OK\n");
      } else {
          response += "\"status\":\"error\",";
          response += "\"message\":\"Calibration not supported or failed for " + String(sensor->getSensorType()) + "\",";
          response += "\"sensor_type\":\"" + String(sensor->getSensorType()) + "\",";
          response += "\"sensor_detected\":true,";
          response += "\"calibration_performed\":false";
          httpStatus = 500;
          DBG_ERROR("Calibration failed: %s\n", sensor->getSensorType());
      }
  }

  response += "}";
  server.send(httpStatus, "application/json", response);
}

void handleSettings() {
  server.send(200, "text/html", getConfigPageHTML());
}

void handleRestart() {
  server.send(200, "text/plain", "Restarting ESP32...");
  delay(1000);
  ESP.restart();
}

void handleConfigReset() {
  DBG_INFO("Resetting config...\n");

  if (SPIFFS.exists(CONFIG_FILE_PATH)) {
    if (SPIFFS.remove(CONFIG_FILE_PATH)) {
      DBG_INFO("Config removed\n");
    } else {
      DBG_ERROR("Remove failed\n");
    }
  }

  createConfigFile();

  JsonDocument doc;
  doc["success"] = true;
  doc["message"] = "Configuration reset to defaults. Restarting...";

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);

  DBG_INFO("Restarting...\n");
  delay(1000);
  ESP.restart();
}

#ifdef ENABLE_ESPNOW
void handleESPNowStatus() {
  JsonDocument doc;

  JsonDocument config = loadConfig();
  bool espnowEnabled = config["espnow_enabled"] | false;
  String forcedMode = config["espnow_force_mode"] | "";
  String actualMode = espnowMgr.getMode();

  doc["enabled"] = espnowEnabled;
  doc["mode"] = actualMode;
  doc["forced_mode"] = forcedMode;
  doc["mac_address"] = espnowMgr.getMACAddress();
  doc["channel"] = (WiFi.status() == WL_CONNECTED) ? WiFi.channel() : 0;

  if (actualMode == "sensor") {
    doc["paired"] = espnowMgr.isPaired();
    doc["peer_count"] = 0;
  } else {
    doc["paired"] = true;
    doc["peer_count"] = espnowMgr.getActivePeerCount();
  }

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}
#endif

void handleRelayList() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for(auto* r : relayMgr.getRelays()) {
        if(r) {
            r->syncState();
            JsonDocument tempDoc;
            deserializeJson(tempDoc, r->getStatusJSON());
            arr.add(tempDoc);
        }
    }

    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
}

void handleRelayToggle() {
    if(!server.hasArg("addr") || !server.hasArg("ch")) {
        server.send(400, "text/plain", "Missing addr or ch param");
        return;
    }

    int addr = server.arg("addr").toInt();
    int ch = server.arg("ch").toInt();

    for(auto* r : relayMgr.getRelays()) {
        if(r->getAddress() == addr) {
            if(r->toggleRelay(ch)) {
                server.send(200, "text/plain", "OK");
            } else {
                server.send(500, "text/plain", "Failed to toggle");
            }
            return;
        }
    }

    server.send(404, "text/plain", "Relay not found");
}
