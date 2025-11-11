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
#include "sensors/SensorFactory.h"

#ifdef ENABLE_RS485
  #include "RS485Manager.h"
  RS485Manager rs485;
#endif

unsigned long lastUpdateCheck = 0;
unsigned long lastSendTime = 0;

#ifndef UNIT_TEST

void setup() {
  Serial.begin(115200);

  Serial.println("Conectado a WiFi");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  if (!SPIFFS.begin(true)) {
    Serial.println("Error montando SPIFFS");
  }

  createConfigFile();

  // Inicializar sensor según build flag
  sensor = SensorFactory::createSensor();
  if (sensor) {
    if (sensor->init()) {
      Serial.printf("Sensor %s inicializado correctamente\n", sensor->getSensorType());
    } else {
      Serial.printf("Error inicializando sensor %s\n", sensor->getSensorType());
    }
  } else {
    Serial.println("Error: No se pudo crear el sensor!");
  }

  #ifdef ENABLE_RS485
    // Inicializar RS485 (sin DE/RE = puenteado para TX permanente)
    rs485.init(16, 17, 9600);  // RX, TX, Baud (DE/RE opcionales)
    Serial.println("RS485 habilitado para transmisión de datos");
  #endif

  clientSecure.setInsecure(); 

  server.on("/actual", HTTP_GET, handleMediciones);
  server.on("/config", HTTP_GET, handleConfiguracion);
  server.on("/config", HTTP_POST, habldePostConfig);
  server.on("/data", HTTP_GET,handleData);
  server.on("/calibrate-scd30", HTTP_GET, handleSCD30Calibration);
    // Add handler for undefined routes
  server.onNotFound([]() {
      // Redirect all undefined routes to root page
      Serial.println("Redirecting to root page for undefined route");
      Serial.printf("Requested route: %s\n", server.uri().c_str());
      server.sendHeader("Location", "/", true);
      server.send(302, "text/plain", "");
  });



  server.enableCORS(true);

  // Optional: Configure timeouts and retries
  wifiManager.setConnectionTimeout(15000);  // 15 seconds
  wifiManager.setMaxRetries(8);
  wifiManager.setValidationTimeout(30000);  // 30 seconds
  wifiManager.init(&server);
    
  Serial.println("Setup complete!");
  Serial.println("Access Point: " + wifiManager.getAPSSID());
  Serial.println("Connect to the AP and go to http://192.168.16.10 to configure WiFi");

  server.begin();
  Serial.println("Servidor web iniciado en el puerto 80");
    
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
  }
  delay(10);
}

#endif  // UNIT_TEST