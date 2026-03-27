#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include "endpoints.h"
#include "globals.h"
#include "constants.h"
#include "configFile.h"
#include "webConfigPage.h"

#include <ArduinoJson.h>

#ifdef SENSOR_MULTI
  #include <vector>
  #include "sensors/ISensor.h"
  // Forward declaration - SensorManager is defined in main.cpp
  class SensorManager;
  extern SensorManager sensorMgr;
  // We need access to getSensors(), so declare it here
  extern std::vector<ISensor*>& getSensorList();
#endif

#ifdef ENABLE_ESPNOW
  #include "ESPNowManager.h"
  extern ESPNowManager espnowMgr;
#endif

void handleMediciones() {
    float temperature = 99, humidity = 100, co2 = 999999, presion = 99;
    String wifiStatus = "unknown";
    bool rotation = false;

    if (sensor && sensor->isActive() && sensor->dataReady() && sensor->read()) {
        temperature = sensor->getTemperature();
        humidity = sensor->getHumidity();
        co2 = sensor->getCO2();
        wifiStatus = (WiFi.status() == WL_CONNECTED) ? "connected" : "disconnected";
    }

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
    doc["a_co2"] = String(co2, 2); // HAY QUE AÑADIR ESTE A LA APLICACION
    doc["wifi_status"] = wifiStatus;  

    String output;
    serializeJsonPretty(doc, output);
    server.send(200, "application/json", output);
}

// Helper to get sensor icon
String getSensorIcon(const char* type) {
    String t = String(type);
    t.toLowerCase();
    if (t.indexOf("scd30") >= 0 || t.indexOf("co2") >= 0) return "🌬️";
    if (t.indexOf("bme") >= 0) return "🌡️";
    if (t.indexOf("onewire") >= 0 || t.indexOf("ds18") >= 0) return "🌡️";
    if (t.indexOf("capacitive") >= 0 || t.indexOf("hd38") >= 0 || t.indexOf("soil") >= 0) return "🌱";
    if (t.indexOf("modbus") >= 0) return "📡";
    return "📊";
}

// Check if sensor is soil moisture type (doesn't have temperature)
bool isSoilSensor(const char* type) {
    String t = String(type);
    t.toLowerCase();
    return (t.indexOf("capacitive") >= 0 || t.indexOf("hd38") >= 0 || t.indexOf("soil") >= 0);
}

void handleData() {
    String wifiStatus = (WiFi.status() == WL_CONNECTED) ? "Conectado" : "Desconectado";
    int wifiRSSI = WiFi.RSSI();

    // HTML header with minimal CSS
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<meta http-equiv='refresh' content='10'>";
    html += "<link rel='icon' type='image/svg+xml' href='/favicon.svg'>";
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
    html += "</style></head><body>";
    html += "<h1>📊 Datos de Sensores</h1>";
    html += "<div class='cards'>";

    int sensorCount = 0;

#ifdef SENSOR_MULTI
    // Multi-sensor mode
    for (auto* s : getSensorList()) {
        if (!s) continue;
        sensorCount++;

        bool isActive = s->isActive();
        bool hasData = isActive && s->dataReady();
        if (hasData) s->read();

        float temp = hasData ? s->getTemperature() : -999;
        float hum = hasData ? s->getHumidity() : -999;
        float co2 = hasData ? s->getCO2() : -999;

        bool hasError = !isActive || !hasData;
        String cardClass = hasError ? " err" : "";

        html += "<div class='card" + cardClass + "'>";
        html += "<div class='hdr'>";
        html += "<span class='type'>" + getSensorIcon(s->getSensorType()) + " " + String(s->getSensorType()) + "</span>";
        html += "<span class='id'>" + String(s->getSensorID()) + "</span>";
        html += "</div>";
        html += "<div class='vals'>";

        bool isSoil = isSoilSensor(s->getSensorType());

        // Temperature (skip for soil sensors)
        if (!isSoil && temp > -100 && temp < 100) {
            String cls = (temp < 10 || temp > 35) ? " warn" : " ok";
            html += "<div class='val" + cls + "'><span>🌡️ Temperatura</span><b>" + String(temp, 1) + "°C</b></div>";
        }

        // Humidity (show as "Humedad de suelo" for soil sensors)
        if (hum >= 0 && hum <= 100) {
            String cls, label;
            if (isSoil) {
                // Soil moisture: low is dry (warn), high is wet (ok)
                cls = (hum < 30) ? " warn" : " ok";
                label = "🌱 Humedad suelo";
            } else {
                cls = (hum < 30 || hum > 80) ? " warn" : " ok";
                label = "💧 Humedad";
            }
            html += "<div class='val" + cls + "'><span>" + label + "</span><b>" + String(hum, 1) + "%</b></div>";
        }

        // CO2 (skip for soil sensors)
        if (!isSoil && co2 > 0 && co2 < 10000) {
            String cls = co2 > 1000 ? " bad" : (co2 > 800 ? " warn" : " ok");
            html += "<div class='val" + cls + "'><span>🌬️ CO₂</span><b>" + String((int)co2) + " ppm</b></div>";
        }

        // If no valid readings
        if ((isSoil || temp <= -100 || temp >= 100) && (hum < 0 || hum > 100) && (isSoil || co2 <= 0 || co2 >= 10000)) {
            if (!isSoil || (hum < 0 || hum > 100)) {
                html += "<div class='val'><span>Estado</span><b>" + String(isActive ? "Sin datos" : "Inactivo") + "</b></div>";
            }
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

        float temp = hasData ? sensor->getTemperature() : -999;
        float hum = hasData ? sensor->getHumidity() : -999;
        float co2 = hasData ? sensor->getCO2() : -999;

        bool hasError = !isActive || !hasData;
        String cardClass = hasError ? " err" : "";

        html += "<div class='card" + cardClass + "'>";
        html += "<div class='hdr'>";
        html += "<span class='type'>" + getSensorIcon(sensor->getSensorType()) + " " + String(sensor->getSensorType()) + "</span>";
        html += "<span class='id'>" + String(sensor->getSensorID()) + "</span>";
        html += "</div>";
        html += "<div class='vals'>";

        bool isSoil = isSoilSensor(sensor->getSensorType());

        if (!isSoil && temp > -100 && temp < 100) {
            String cls = (temp < 10 || temp > 35) ? " warn" : " ok";
            html += "<div class='val" + cls + "'><span>🌡️ Temperatura</span><b>" + String(temp, 1) + "°C</b></div>";
        }
        if (hum >= 0 && hum <= 100) {
            String cls, label;
            if (isSoil) {
                cls = (hum < 30) ? " warn" : " ok";
                label = "🌱 Humedad suelo";
            } else {
                cls = (hum < 30 || hum > 80) ? " warn" : " ok";
                label = "💧 Humedad";
            }
            html += "<div class='val" + cls + "'><span>" + label + "</span><b>" + String(hum, 1) + "%</b></div>";
        }
        if (!isSoil && co2 > 0 && co2 < 10000) {
            String cls = co2 > 1000 ? " bad" : (co2 > 800 ? " warn" : " ok");
            html += "<div class='val" + cls + "'><span>🌬️ CO₂</span><b>" + String((int)co2) + " ppm</b></div>";
        }
        if ((isSoil || temp <= -100 || temp >= 100) && (hum < 0 || hum > 100) && (isSoil || co2 <= 0 || co2 >= 10000)) {
            if (!isSoil || (hum < 0 || hum > 100)) {
                html += "<div class='val'><span>Estado</span><b>" + String(isActive ? "Sin datos" : "Inactivo") + "</b></div>";
            }
        }

        html += "</div></div>";
    }
#endif

    html += "</div>";

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

    // Add dynamic data not stored in the file
    if (WiFi.status() == WL_CONNECTED) {
        doc["current_wifi_channel"] = WiFi.channel();
    } else {
        doc["current_wifi_channel"] = 0; // 0 indicates not connected or channel not available
    }

    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
}

void habldePostConfig() {
    Serial.println("Updating configuration...");

    if (!server.hasArg("plain")) {
      server.send(400, "text/plain", "No JSON data received");
      return;
    }

    String jsonString = server.arg("plain");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonString);

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      server.send(400, "text/plain", "Invalid JSON format");
      return;
    }

    // Update WiFi if SSID changed
    const char* new_ssid = doc["ssid"];
    const char* new_password = doc["passwd"];

    if (new_ssid && strlen(new_ssid) > 0 && strcmp(new_ssid, "ToChange") != 0) {
      wifiManager.onChange(String(new_ssid), String(new_password));
      Serial.printf("WiFi updated: %s\n", new_ssid);
    }

    // Save complete config to SPIFFS
    if (updateConfig(doc)) {
      server.send(200, "text/plain", "Configuration updated successfully. Some changes require restart.");
      Serial.println("Configuration saved to SPIFFS");
    } else {
      server.send(500, "text/plain", "Failed to save configuration");
      Serial.println("Failed to save configuration");
    }
  }

void handleSCD30Calibration() {
  Serial.printf("Endpoint /calibrate-scd30 called for sensor: %s\n",
                sensor ? sensor->getSensorType() : "NULL");

  String response = "{";
  int httpStatus = 200;

  if (!sensor || !sensor->isActive()) {
      response += "\"status\":\"error\",";
      response += "\"message\":\"No sensor active\",";
      response += "\"sensor_detected\":false,";
      response += "\"calibration_performed\":false";
      httpStatus = 503;
  } else {
      // Intentar calibración (solo algunos sensores la soportan)
      bool calibrationSuccess = sensor->calibrate(400);

      if (calibrationSuccess) {
          response += "\"status\":\"success\",";
          response += "\"message\":\"Sensor calibration completed successfully\",";
          response += "\"sensor_type\":\"" + String(sensor->getSensorType()) + "\",";
          response += "\"sensor_detected\":true,";
          response += "\"calibration_performed\":true,";
          response += "\"target_co2\":400,";
          response += "\"note\":\"Allow 2-3 minutes for sensor to stabilize after calibration\"";
          Serial.println("Sensor calibration successful!");
      } else {
          response += "\"status\":\"error\",";
          response += "\"message\":\"Calibration not supported or failed for " + String(sensor->getSensorType()) + "\",";
          response += "\"sensor_type\":\"" + String(sensor->getSensorType()) + "\",";
          response += "\"sensor_detected\":true,";
          response += "\"calibration_performed\":false";
          httpStatus = 500;
          Serial.printf("Sensor %s calibration failed or not supported\n", sensor->getSensorType());
      }
  }

  response += "}";

  server.send(httpStatus, "application/json", response);
  Serial.println("Calibration response sent: " + response);
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
  Serial.println("[→ INFO] Reseteando configuración completa...");

  // Remove existing config file
  if (SPIFFS.exists(CONFIG_FILE_PATH)) {
    if (SPIFFS.remove(CONFIG_FILE_PATH)) {
      Serial.println("[✓ OK  ] Config.json eliminado");
    } else {
      Serial.println("[✗ ERR ] Error al eliminar config.json");
    }
  }

  // Create new default config
  createConfigFile();

  // Send response
  JsonDocument doc;
  doc["success"] = true;
  doc["message"] = "Configuration reset to defaults. Restarting...";

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);

  Serial.println("[→ INFO] Reiniciando ESP32 con configuración por defecto...");
  delay(1000);
  ESP.restart();
}

#ifdef ENABLE_ESPNOW
void handleESPNowStatus() {
  JsonDocument doc;

  JsonDocument config = loadConfig();
  bool espnowEnabled = config["espnow_enabled"] | false;
  String forcedMode = config["espnow_force_mode"] | "";
  String actualMode = espnowMgr.getMode();  // Get actual running mode

  doc["enabled"] = espnowEnabled;
  doc["mode"] = actualMode;  // Show actual mode (after auto-detection)
  doc["forced_mode"] = forcedMode;  // Show configured forced mode
  doc["mac_address"] = espnowMgr.getMACAddress();
  doc["channel"] = (WiFi.status() == WL_CONNECTED) ? WiFi.channel() : 0;

  if (actualMode == "sensor") {
    doc["paired"] = espnowMgr.isPaired();
    doc["peer_count"] = 0;  // Sensors don't track peers
  } else {
    doc["paired"] = true;  // Gateway is always "paired"
    doc["peer_count"] = espnowMgr.getActivePeerCount();
  }

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}
#endif