#ifndef SENSOR_SCD30_H
#define SENSOR_SCD30_H

#include "ISensor.h"
#include <Adafruit_SCD30.h>

class SensorSCD30 : public ISensor {
private:
    Adafruit_SCD30 scd30;
    bool active;
    float temperature;
    float humidity;
    float co2;

public:
    SensorSCD30() : active(false), temperature(99), humidity(100), co2(999999) {}

    bool init() override {
        active = scd30.begin();
        if (!active) {
            Serial.println("No se pudo inicializar el sensor SCD30!");
        }
        return active;
    }

    bool dataReady() override {
        return active && scd30.dataReady();
    }

    bool read() override {
        if (!active) return false;

        if (!scd30.read()) {
            Serial.println("Error leyendo el sensor SCD30!");
            return false;
        }

        temperature = scd30.temperature;
        humidity = scd30.relative_humidity;
        co2 = scd30.CO2;
        return true;
    }

    float getTemperature() override { return temperature; }
    float getHumidity() override { return humidity; }
    float getCO2() override { return co2; }
    const char* getSensorType() override { return "SCD30"; }

    bool calibrate(float reference = 400) override {
        if (!active) return false;
        return scd30.forceRecalibrationWithReference((uint16_t)reference);
    }

    bool isActive() override { return active; }
};

#endif // SENSOR_SCD30_H
