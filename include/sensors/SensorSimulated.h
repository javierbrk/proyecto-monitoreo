#ifndef SENSOR_SIMULATED_H
#define SENSOR_SIMULATED_H

#include "ISensor.h"
#include <Arduino.h>

class SensorSimulated : public ISensor {
private:
    bool active;
    float temperature;
    float humidity;
    float co2;

public:
    SensorSimulated() : active(false), temperature(22.5), humidity(50), co2(400) {}

    bool init() override {
        active = true;
        Serial.println("Modo simulación activado - datos aleatorios");
        return true;
    }

    bool dataReady() override {
        return active;
    }

    bool read() override {
        if (!active) return false;

        // Generate random variations
        temperature = 22.5 + random(-100, 100) * 0.01;
        humidity = 50 + random(-500, 500) * 0.01;
        co2 = 400 + random(0, 200);

        return true;
    }

    float getTemperature() override { return temperature; }
    float getHumidity() override { return humidity; }
    float getCO2() override { return co2; }
    const char* getSensorType() override { return "Simulated"; }
    const char* getSensorID() override { return "sim-001"; }
    const char* getMeasurementsString() override {
        static char measString[64];
        snprintf(measString, sizeof(measString), "temp=%.2f,hum=%.2f,co2=%.2f", temperature, humidity, co2);
        return measString;
    }
    bool isActive() override { return active; }
};

#endif // SENSOR_SIMULATED_H
