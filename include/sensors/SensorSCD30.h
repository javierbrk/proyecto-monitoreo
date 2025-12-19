#ifndef SENSOR_SCD30_H
#define SENSOR_SCD30_H

#include "ISensor.h"
#include "ITemperatureSensor.h"
#include "IHumiditySensor.h"
#include "ICO2Sensor.h"
#include <Adafruit_SCD30.h>
#include "../debug.h"

class SensorSCD30 : public ITemperatureSensor, public IHumiditySensor, public ICO2Sensor {
private:
    Adafruit_SCD30 scd30;
    bool active;
    float temperature;
    float humidity;
    float co2;

public:
    SensorSCD30() : active(false), temperature(999), humidity(100), co2(999999) {}

    bool init() override {
        active = scd30.begin();
        if (!active) {
            DBG_ERROR("[SCD30] Init failed\n");
        } else {
            DBG_INFO("[SCD30] OK\n");
        }
        return active;
    }

    bool dataReady() override {
        return active && scd30.dataReady();
    }

    bool read() override {
        if (!active) return false;

        if (!scd30.read()) {
            DBG_ERROR("[SCD30] Read error\n");
            return false;
        }

        temperature = scd30.temperature;
        humidity = scd30.relative_humidity;
        co2 = scd30.CO2;
        return true;
    }

    // ITemperatureSensor
    float getTemperature() override { return temperature; }

    // IHumiditySensor
    float getHumidity() override { return humidity; }

    // ICO2Sensor
    float getCO2() override { return co2; }

    const char* getSensorType() override { return "SCD30"; }

    const char* getSensorID() override {
        static char sensorId[16];
        snprintf(sensorId, sizeof(sensorId), "thc-i2c-0x%02X", SCD30_I2CADDR_DEFAULT);
        return sensorId;
    }

    const char* getMeasurementsString() override {
        static char measString[64];
        snprintf(measString, sizeof(measString), "temp=%.1f,hum=%.1f,co2=%.0f", temperature, humidity, co2);
        return measString;
    }

    bool calibrate(float reference = 400) override {
        if (!active) return false;
        return scd30.forceRecalibrationWithReference((uint16_t)reference);
    }

    bool isActive() override { return active; }
};

#endif // SENSOR_SCD30_H
