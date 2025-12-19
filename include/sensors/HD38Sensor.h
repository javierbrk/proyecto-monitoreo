#ifndef HD38_SENSOR_H
#define HD38_SENSOR_H

#include "ISensor.h"
#include "IMoistureSensor.h"
#include <Arduino.h>
#include "../debug.h"

/**
 * Sensor HD-38 - Soil Moisture / Rain Sensor
 *
 * Features:
 *   - LM393 comparator IC
 *   - Analog output: 0-5V (requires 2:1 divider for ESP32 3.3V ADC)
 *   - Digital output: 0/5V with adjustable threshold via potentiometer
 *
 * Wiring (analog with voltage divider):
 *   Sensor AOUT -> Divider (10k+10k) -> ESP32 ADC pin
 *   Sensor VCC  -> 5V
 *   Sensor GND  -> GND
 */
class HD38Sensor : public IMoistureSensor {
private:
    int analogPin;
    int digitalPin;
    bool useVoltageDivider;
    bool invertLogic;
    float moisture;
    bool digitalState;
    bool active;
    int dryValue;
    int wetValue;
    String sensorName;

public:
    HD38Sensor(int aPin = 35,
               int dPin = -1,
               bool voltageDivider = true,
               bool invert = false,
               const char* name = "HD38")
        : analogPin(aPin),
          digitalPin(dPin),
          useVoltageDivider(voltageDivider),
          invertLogic(invert),
          moisture(0),
          digitalState(false),
          active(false),
          dryValue(4095),
          wetValue(0),
          sensorName(name) {}

    bool init() override {
        DBG_VERBOSE("[HD38] '%s': a=%d d=%d div=%s\n",
                      sensorName.c_str(), analogPin, digitalPin,
                      useVoltageDivider ? "y" : "n");

        if (analogPin >= 0) {
            pinMode(analogPin, INPUT);
            analogReadResolution(12);
        }

        if (digitalPin >= 0) {
            pinMode(digitalPin, INPUT);
        }

        if (analogPin < 0 && digitalPin < 0) {
            DBG_ERROR("[HD38] No pins configured\n");
            active = false;
            return false;
        }

        active = true;
        DBG_INFO("[HD38] OK\n");
        return true;
    }

    bool dataReady() override {
        return active;
    }

    bool read() override {
        if (!active) return false;

        if (analogPin >= 0) {
            int rawValue = analogRead(analogPin);

            if (useVoltageDivider) {
                rawValue = constrain(rawValue, 0, 3100);
            }

            moisture = map(rawValue, dryValue, wetValue, 0, 100);
            moisture = constrain(moisture, 0, 100);

            DBG_VERBOSE("[HD38] '%s' Raw=%d M=%.1f%%\n",
                         sensorName.c_str(), rawValue, moisture);
        }

        if (digitalPin >= 0) {
            digitalState = digitalRead(digitalPin);
            if (invertLogic) {
                digitalState = !digitalState;
            }
            DBG_VERBOSE("[HD38] '%s' D=%s\n",
                         sensorName.c_str(), digitalState ? "WET" : "DRY");
        }

        return true;
    }

    // IMoistureSensor
    float getMoisture() override { return moisture; }

    const char* getSensorType() override {
        static char typeName[32];
        snprintf(typeName, sizeof(typeName), "hd38_%s", sensorName.c_str());
        return typeName;
    }

    const char* getSensorID() override {
        static char sensorId[32];
        snprintf(sensorId, sizeof(sensorId), "m-adc-%d", analogPin);
        return sensorId;
    }

    const char* getMeasurementsString() override {
        static char measString[64];
        snprintf(measString, sizeof(measString), "moisture=%.1f", moisture);
        return measString;
    }

    bool isActive() override { return active; }

    bool isWet() { return digitalState; }

    void setCalibration(int dry, int wet) {
        dryValue = dry;
        wetValue = wet;
        DBG_INFO("[HD38] Cal: dry=%d wet=%d\n", dry, wet);
    }

    int getRawValue() {
        if (analogPin >= 0) {
            return analogRead(analogPin);
        }
        return -1;
    }
};

#endif // HD38_SENSOR_H
