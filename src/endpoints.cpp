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

void handleData() {
    float temperature = 99, humidity = 100, co2 = 999999, presion = 99;
    String wifiStatus = "unknown";
    bool rotation = false;

    if (sensor && sensor->isActive() && sensor->dataReady() && sensor->read()) {
        temperature = sensor->getTemperature();
        humidity = sensor->getHumidity();
        co2 = sensor->getCO2();
        wifiStatus = (WiFi.status() == WL_CONNECTED) ? "connected" : "disconnected";
    }
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='10'>"; // refresh every 10 seconds
  html += "<title>Sensor Data</title>";
  html += "<style>";
  html += "body{font-family:Arial, sans-serif; text-align:center; background:#f4f4f4;}";
  html += "h1{color:#333;}";
  html += "table{margin:auto; border-collapse:collapse;}";
  html += "td,th{padding:8px 15px; border:1px solid #ccc;}";
  html += "</style></head><body>";
  html += "<h1>" + String(sensor ? sensor->getSensorType() : "Unknown") + " Sensor Data</h1>";
  html += "<table>";
  html += "<tr><th>Temperature (°C)</th><th>Humidity (%)</th><th>CO₂ (ppm)</th><th>wifi</th></tr>";
  html += "<tr>";
  html += "<td>" + String(temperature, 2) + "</td>";
  html += "<td>" + String(humidity, 2) + "</td>";
  html += "<td>" + String(co2, 2) + "</td>";
  html += "<td>" + wifiStatus + "</td>";
  html += "</tr></table>";
  html += "<p>Last update: " + String(millis() / 1000) + "s since start</p>";
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