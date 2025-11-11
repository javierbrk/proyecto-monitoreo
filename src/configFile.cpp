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

