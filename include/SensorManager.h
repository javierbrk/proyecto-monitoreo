#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <vector>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "sensors/ISensor.h"
#include "sensors/SensorSCD30.h"
#include "sensors/SensorCapacitive.h"
#include "sensors/SensorBME280.h"
#include "sensors/SensorSimulated.h"
#include "sensors/SensorOneWire.h"
#include "sensors/ModbusTHSensor.h"
#include "sensors/HD38Sensor.h"

class SensorManager {
private:
    std::vector<ISensor*> sensors;
    std::vector<DallasTemperature*> dallasInstances;  // Para cleanup

public:
    SensorManager() {}

    ~SensorManager() {
        // Cleanup
        for (auto* sensor : sensors) {
            delete sensor;
        }
        for (auto* dallas : dallasInstances) {
            delete dallas;
        }
    }

    void loadFromConfig(JsonDocument& config) {
        if (!config["sensors"].is<JsonArray>()) {
            Serial.println("No sensors config found, using default capacitive");
            sensors.push_back(new SensorCapacitive());
            sensors[0]->init();
            return;
        }

        JsonArray sensorsArray = config["sensors"];

        for (JsonObject sensorCfg : sensorsArray) {
            bool enabled = sensorCfg["enabled"] | true;
            if (!enabled) continue;

            const char* type = sensorCfg["type"];
            JsonObject cfg = sensorCfg["config"];

            if (strcmp(type, "capacitive") == 0) {
                int pin = cfg["pin"] | 34;
                ISensor* s = new SensorCapacitive(pin);
                if (s->init()) {
                    sensors.push_back(s);
                    Serial.printf("Capacitive sensor on pin %d added\n", pin);
                }

            } else if (strcmp(type, "scd30") == 0) {
                ISensor* s = new SensorSCD30();
                if (s->init()) {
                    sensors.push_back(s);
                    Serial.println("SCD30 sensor added");
                }

            } else if (strcmp(type, "bme280") == 0) {
                ISensor* s = new SensorBME280();
                if (s->init()) {
                    sensors.push_back(s);
                    Serial.println("BME280 sensor added");
                }

            } else if (strcmp(type, "simulated") == 0) {
                ISensor* s = new SensorSimulated();
                if (s->init()) {
                    sensors.push_back(s);
                    Serial.println("Simulated sensor added");
                }

            } else if (strcmp(type, "onewire") == 0) {
                int pin = cfg["pin"] | 4;
                bool scan = cfg["scan"] | true;
                if (scan) {
                    int count = scanOneWire(pin);
                    Serial.printf("OneWire: %d sensors detected on pin %d\n", count, pin);
                }

            } else if (strcmp(type, "modbus_th") == 0) {
                // Modbus RTU Temperature/Humidity sensor (TH-MB-04S)
                uint8_t addr = cfg["address"] | 1;
                int rx = cfg["rx_pin"] | 16;
                int tx = cfg["tx_pin"] | 17;
                int de = cfg["de_pin"] | -1;
                uint32_t baud = cfg["baudrate"] | 9600;

                ISensor* s = new ModbusTHSensor(addr, rx, tx, de, baud);
                if (s->init()) {
                    sensors.push_back(s);
                    Serial.printf("ModbusTH sensor (addr=%d) added\n", addr);
                } else {
                    delete s;
                    Serial.printf("ModbusTH sensor (addr=%d) init failed\n", addr);
                }

            } else if (strcmp(type, "hd38") == 0) {
                // HD-38 Soil Moisture sensor
                int aPin = cfg["analog_pin"] | 35;
                int dPin = cfg["digital_pin"] | -1;
                bool divider = cfg["voltage_divider"] | true;
                bool invert = cfg["invert_logic"] | false;
                const char* name = cfg["name"] | "HD38";

                ISensor* s = new HD38Sensor(aPin, dPin, divider, invert, name);
                if (s->init()) {
                    sensors.push_back(s);
                    Serial.printf("HD38 sensor '%s' on pin %d added\n", name, aPin);
                } else {
                    delete s;
                    Serial.println("HD38 sensor init failed");
                }
            }
        }

        Serial.printf("Total sensors active: %d\n", sensors.size());
    }

    int scanOneWire(int pin) {
        OneWire* oneWire = new OneWire(pin);
        DallasTemperature* dallas = new DallasTemperature(oneWire);
        dallas->begin();

        dallasInstances.push_back(dallas);  // Store for cleanup

        int deviceCount = dallas->getDeviceCount();

        for (int i = 0; i < deviceCount; i++) {
            DeviceAddress addr;
            if (dallas->getAddress(addr, i)) {
                ISensor* s = new SensorOneWire(dallas, addr, i);
                if (s->init()) {
                    sensors.push_back(s);
                }
            }
        }

        // Request temperatures for all devices (async)
        dallas->requestTemperatures();

        return deviceCount;
    }

    void readAll() {
        // Request temperatures from all OneWire sensors first (async)
        for (auto* dallas : dallasInstances) {
            dallas->requestTemperatures();
        }

        // Small delay for OneWire conversion
        if (!dallasInstances.empty()) {
            delay(100);
        }

        // Read all sensors
        for (auto* s : sensors) {
            if (s->isActive() && s->dataReady()) {
                s->read();
            }
        }
    }

    std::vector<ISensor*>& getSensors() {
        return sensors;
    }

    int getSensorCount() const {
        return sensors.size();
    }

    // Get sensor identifier for logging
    String getSensorId(ISensor* sensor) const {
        if (strcmp(sensor->getSensorType(), "OneWire") == 0) {
            return ((SensorOneWire*)sensor)->getSensorId();
        } else if (strcmp(sensor->getSensorType(), "Capacitive") == 0) {
            return String(sensor->getSensorType()) + "_default";
        }
        return String(sensor->getSensorType());
    }
};

#endif // SENSOR_MANAGER_H
