#ifndef SENSOR_CAPACITIVE_H
#define SENSOR_CAPACITIVE_H

#include "ISensor.h"
#include "IMoistureSensor.h"
#include <Arduino.h>
#include "../debug.h"

#define CAPACITIVE_PIN 34
#define ADC_MAX 4095
#define ADC_MIN 0

class SensorCapacitive : public IMoistureSensor {
private:
    int pin;
    float moisture;
    bool active;
    int dryValue;
    int wetValue;

public:
    SensorCapacitive(int adcPin = CAPACITIVE_PIN, int dry = ADC_MAX, int wet = ADC_MIN)
        : pin(adcPin), moisture(0), active(false), dryValue(dry), wetValue(wet) {}

    bool init() override {
        pinMode(pin, INPUT);
        active = true;
        DBG_INFO("[Capacitive] pin %d OK\n", pin);
        return true;
    }

    bool dataReady() override {
        return active;
    }

    bool read() override {
        if (!active) return false;

        int rawValue = analogRead(pin);
        moisture = map(rawValue, dryValue, wetValue, 0, 100);
        moisture = constrain(moisture, 0, 100);

        DBG_VERBOSE("[Capacitive] Raw=%d M=%.1f%%\n", rawValue, moisture);
        return true;
    }

    // IMoistureSensor
    float getMoisture() override { return moisture; }

    const char* getSensorType() override { return "Capacitive"; }

    const char* getSensorID() override {
        static char idString[16];
        snprintf(idString, sizeof(idString), "m-adc-%d", pin);
        return idString;
    }

    const char* getMeasurementsString() override {
        static char measString[32];
        snprintf(measString, sizeof(measString), "moisture=%.1f", moisture);
        return measString;
    }

    bool isActive() override { return active; }

    void setCalibration(int dry, int wet) {
        dryValue = dry;
        wetValue = wet;
        DBG_INFO("[Capacitive] Cal: dry=%d wet=%d\n", dry, wet);
    }
};

#endif // SENSOR_CAPACITIVE_H
