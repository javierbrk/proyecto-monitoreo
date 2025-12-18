#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "configFile.h"
#include "globals.h"
#include "constants.h"



void createConfigFile() {
    
    //SPIFFS.remove(path);

    if (SPIFFS.exists(CONFIG_FILE_PATH)) {
      Serial.println("Archivo de configuración ya existe.");
      return;
    }
  
    Serial.println("Creando archivo de configuración...");
  
    File file = SPIFFS.open(CONFIG_FILE_PATH, FILE_WRITE);
    if (!file) {
      Serial.println("Error al abrir config.json para escritura.");
      return;
    }
  
    JsonDocument config;
    config["max_temperature"] = 37.7;
    config["min_temperature"] = 37.3;
    config["rotation_duration"] = 50000;
    config["rotation_period"] = 3600000;
    config["ssid"] = "ToChange";
    config["passwd"] = "ToChange";
    config["tray_one_date"] = 0;
    config["tray_two_date"] = 0;
    config["tray_three_date"] = 0;
    config["incubation_period"] = 18;
    config["max_hum"] = 65;
    config["min_hum"] = 55;

    String mac = WiFi.macAddress();
    mac.replace(":", "");
    config["hash"] = mac;
    config["incubator_name"] = "moni-" + mac; // en algun momento se debe a cambiar por el nombre del incubador por moni

    // Configuración de sensores (para modo multi-sensor)
    JsonArray sensors = config["sensors"].to<JsonArray>();

    JsonObject cap1 = sensors.add<JsonObject>();
    cap1["type"] = "capacitive";
    cap1["enabled"] = true;
    JsonObject cap1Cfg = cap1["config"].to<JsonObject>();
    cap1Cfg["pin"] = 34;
    cap1Cfg["name"] = "Soil1";

    JsonObject onewire = sensors.add<JsonObject>();
    onewire["type"] = "onewire";
    onewire["enabled"] = false;  // Disabled by default
    JsonObject onewireCfg = onewire["config"].to<JsonObject>();
    onewireCfg["pin"] = 4;
    onewireCfg["scan"] = true;

    // Configuración RS485
    config["rs485_enabled"] = false;
    config["rs485_rx"] = 16;
    config["rs485_tx"] = 17;
    config["rs485_baud"] = 9600;

    // Configuración ESP-NOW (auto-adaptativo)
    config["espnow_enabled"] = false;
    config["espnow_force_mode"] = "";  // "" = auto-detect, "gateway" o "sensor" = force mode
    config["espnow_channel"] = 1;      // WiFi channel (1-13)
    config["beacon_interval_ms"] = 2000;
    config["discovery_timeout_ms"] = 15000;
    config["send_interval_ms"] = 30000;
    config["grafana_ping_url"] = "http://192.168.1.1/ping";  // URL for connectivity test

    if (serializeJsonPretty(config, file) == 0) {
      Serial.println("Error al escribir JSON en archivo.");
    } else {
      Serial.println("Archivo config.json creado correctamente.");
    }

    file.close();
  }

String getConfigFile() {
  File file = SPIFFS.open(CONFIG_FILE_PATH, FILE_READ);
  if (!file || file.isDirectory()) {
      Serial.println("Error al abrir config.json");
      return String();
  }
  String json = file.readString();
  file.close();
  return json;
}

JsonDocument loadConfig() {
  JsonDocument doc;

  File file = SPIFFS.open(CONFIG_FILE_PATH, FILE_READ);
  if (!file || file.isDirectory()) {
      Serial.println("Error al abrir config.json para lectura");
      return doc;  // Empty document
  }

  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
      Serial.print("Error deserializando config.json: ");
      Serial.println(error.c_str());
      return doc;
  }

  // Automatic migration: add sensors array if missing
  if (!doc["sensors"].is<JsonArray>() || doc["sensors"].size() == 0) {
      Serial.println("[→ INFO] Migrando configuración: agregando sensores por defecto");

      JsonArray sensors = doc["sensors"].to<JsonArray>();

      // Add SCD30 as default (enabled)
      JsonObject scd30 = sensors.add<JsonObject>();
      scd30["type"] = "scd30";
      scd30["enabled"] = true;
      scd30["config"].to<JsonObject>();

      // Add other sensors (disabled by default)
      JsonObject bme280 = sensors.add<JsonObject>();
      bme280["type"] = "bme280";
      bme280["enabled"] = false;
      bme280["config"].to<JsonObject>();

      JsonObject cap = sensors.add<JsonObject>();
      cap["type"] = "capacitive";
      cap["enabled"] = false;
      JsonObject capCfg = cap["config"].to<JsonObject>();
      capCfg["pin"] = 34;
      capCfg["name"] = "Soil1";

      JsonObject onewire = sensors.add<JsonObject>();
      onewire["type"] = "onewire";
      onewire["enabled"] = false;
      JsonObject onewireCfg = onewire["config"].to<JsonObject>();
      onewireCfg["pin"] = 4;
      onewireCfg["scan"] = true;

      // Save migrated config
      if (updateConfig(doc)) {
          Serial.println("[✓ OK  ] Configuración migrada exitosamente");
      } else {
          Serial.println("[✗ ERR ] Error al guardar configuración migrada");
      }
  }

  return doc;
}

bool updateConfig(JsonDocument& newConfig) {
  // Remove existing config file
  if (SPIFFS.exists(CONFIG_FILE_PATH)) {
    SPIFFS.remove(CONFIG_FILE_PATH);
  }

  // Write new config
  File file = SPIFFS.open(CONFIG_FILE_PATH, FILE_WRITE);
  if (!file) {
    Serial.println("Error al abrir config.json para escritura");
    return false;
  }

  if (serializeJsonPretty(newConfig, file) == 0) {
    Serial.println("Error al escribir JSON actualizado");
    file.close();
    return false;
  }

  file.close();
  Serial.println("Configuración actualizada correctamente");
  return true;
}

