#ifndef SENSOR_CAPACITIVE_H
#define SENSOR_CAPACITIVE_H

#include "ISensor.h"
#include <Arduino.h>

#define CAPACITIVE_PIN 34  // ADC pin for capacitive soil moisture sensor
#define ADC_MAX 4095       // 12-bit ADC
#define ADC_MIN 0

class SensorCapacitive : public ISensor {
private:
    int pin;
    float humidity;  // Soil moisture percentage
    bool active;

    // Calibration values (can be adjusted per sensor)
    int dryValue;    // ADC value when completely dry
    int wetValue;    // ADC value when completely wet

public:
    SensorCapacitive(int adcPin = CAPACITIVE_PIN, int dry = ADC_MAX, int wet = ADC_MIN)
        : pin(adcPin), humidity(0), active(false), dryValue(dry), wetValue(wet) {}

    bool init() override {
        pinMode(pin, INPUT);
        active = true;
        Serial.printf("Sensor capacitivo inicializado en pin %d\n", pin);
        return true;
    }

    bool dataReady() override {
        // ADC always ready
        return active;
    }

    bool read() override {
        if (!active) return false;

        int rawValue = analogRead(pin);

        // Map ADC value to 0-100% (inverted: higher ADC = drier = lower %)
        humidity = map(rawValue, dryValue, wetValue, 0, 100);

        // Constrain to valid range
        humidity = constrain(humidity, 0, 100);

        Serial.printf("Raw ADC: %d, Humedad suelo: %.1f%%\n", rawValue, humidity);
        return true;
    }

    float getTemperature() override {
        return -1;  // Not available
    }

    float getHumidity() override {
        return humidity;  // Soil moisture
    }

    float getCO2() override {
        return -1;  // Not available
    }

    const char* getSensorType() override {
        return "Capacitive";
    }

    const char* getSensorID() override {
        static char idString[16];
        snprintf(idString, sizeof(idString), "h-adc-%d", pin);
        return idString;
    }

    const char* getMeasurementsString() override {
        static char measString[32];
        snprintf(measString, sizeof(measString), "soil_hum=%.1f", humidity);
        return measString;
    }

    bool isActive() override {
        return active;
    }

    // Calibration: set dry and wet reference values
    void setCalibration(int dry, int wet) {
        dryValue = dry;
        wetValue = wet;
        Serial.printf("Calibración actualizada: Dry=%d, Wet=%d\n", dry, wet);
    }
};

#endif // SENSOR_CAPACITIVE_H
