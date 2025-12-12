#ifndef HD38_SENSOR_H
#define HD38_SENSOR_H

#include "ISensor.h"
#include <Arduino.h>

/**
 * Sensor HD-38 - Soil Moisture / Rain Sensor
 *
 * Características:
 *   - LM393 comparator IC
 *   - Salida Analog: 0-5V (requiere divisor 2:1 para ESP32 3.3V ADC)
 *   - Salida Digital: 0/5V con threshold ajustable via potenciómetro
 *   - Cable: 5m con sondas gold-coated
 *   - Alimentación: 3.3-5V DC @ 15mA
 *
 * Wiring (usando salida analog con divisor de voltaje):
 *   Sensor AOUT → Divisor (10k+10k) → ESP32 ADC pin
 *   Sensor VCC  → 5V
 *   Sensor GND  → GND
 *
 * Wiring (usando salida digital):
 *   Sensor DOUT → ESP32 GPIO
 *   Sensor VCC  → 3.3V o 5V
 *   Sensor GND  → GND
 *
 * Config:
 *   - analogPin: GPIO ADC para lectura analog (default 35)
 *   - digitalPin: GPIO para lectura digital (optional, -1 disabled)
 *   - useVoltageDivider: true si hay divisor 2:1 (default true)
 *   - invertLogic: true si seco=HIGH húmedo=LOW (default false)
 */
class HD38Sensor : public ISensor {
private:
    int analogPin;
    int digitalPin;
    bool useVoltageDivider;
    bool invertLogic;

    float humidity;      // Soil moisture percentage (analog)
    bool digitalState;   // Digital threshold state
    bool active;

    // Calibration values for analog
    int dryValue;    // ADC value when dry
    int wetValue;    // ADC value when wet

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
          humidity(0),
          digitalState(false),
          active(false),
          dryValue(4095),  // Max ADC = dry
          wetValue(0),     // Min ADC = wet
          sensorName(name) {}

    bool init() override {
        Serial.printf("[HD38] Initializing '%s': analog=%d, digital=%d, divider=%s\n",
                      sensorName.c_str(), analogPin, digitalPin,
                      useVoltageDivider ? "yes" : "no");

        // Configure analog pin
        if (analogPin >= 0) {
            pinMode(analogPin, INPUT);
            analogReadResolution(12);  // 12-bit ADC (0-4095)
        }

        // Configure digital pin
        if (digitalPin >= 0) {
            pinMode(digitalPin, INPUT);
        }

        // Verify at least one input is configured
        if (analogPin < 0 && digitalPin < 0) {
            Serial.println("[HD38] Error: no input pins configured");
            active = false;
            return false;
        }

        active = true;
        Serial.println("[HD38] Initialized successfully");
        return true;
    }

    bool dataReady() override {
        return active;
    }

    bool read() override {
        if (!active) return false;

        // Read analog value
        if (analogPin >= 0) {
            int rawValue = analogRead(analogPin);

            // If using voltage divider (5V→2.5V max), scale reading
            // With 2:1 divider, 5V input = 2.5V = ~3100 ADC at 3.3V ref
            // Adjust dryValue/wetValue accordingly
            if (useVoltageDivider) {
                // Scale factor for 2:1 divider with 5V sensor
                // Max expected: 2.5V / 3.3V * 4095 ≈ 3100
                rawValue = constrain(rawValue, 0, 3100);
            }

            // Map to 0-100% humidity
            // Invert: higher ADC = drier = lower humidity
            humidity = map(rawValue, dryValue, wetValue, 0, 100);
            humidity = constrain(humidity, 0, 100);

            Serial.printf("[HD38] '%s' Raw=%d, Humidity=%.1f%%\n",
                         sensorName.c_str(), rawValue, humidity);
        }

        // Read digital value
        if (digitalPin >= 0) {
            digitalState = digitalRead(digitalPin);

            if (invertLogic) {
                digitalState = !digitalState;
            }

            Serial.printf("[HD38] '%s' Digital=%s\n",
                         sensorName.c_str(),
                         digitalState ? "WET" : "DRY");
        }

        return true;
    }

    float getTemperature() override {
        return -1;  // Not available
    }

    float getHumidity() override {
        return humidity;
    }

    float getCO2() override {
        return -1;  // Not available
    }

    const char* getSensorType() override {
        static char typeName[32];
        snprintf(typeName, sizeof(typeName), "hd38_%s", sensorName.c_str());
        return typeName;
    }

    const char* getSensorID() override {
        static char sensorId[32];
        snprintf(sensorId, sizeof(sensorId), "h-adc-%d", analogPin);
        return sensorId;
    }

    const char* getMeasurementsString() override {
        static char measString[64];
        snprintf(measString, sizeof(measString), "Soil_HUM=%.1f", humidity);
        return measString;
    }

    bool isActive() override {
        return active;
    }

    /**
     * Get digital threshold state
     * Returns true if moisture exceeds threshold (wet)
     */
    bool isWet() {
        return digitalState;
    }

    /**
     * Calibration: set dry and wet ADC reference values
     *
     * To calibrate:
     * 1. Put sensor in air (dry): note ADC value → dryValue
     * 2. Put sensor in water (wet): note ADC value → wetValue
     */
    void setCalibration(int dry, int wet) {
        dryValue = dry;
        wetValue = wet;
        Serial.printf("[HD38] Calibration: dry=%d, wet=%d\n", dry, wet);
    }

    /**
     * Get raw ADC value for calibration
     */
    int getRawValue() {
        if (analogPin >= 0) {
            return analogRead(analogPin);
        }
        return -1;
    }
};

#endif // HD38_SENSOR_H
