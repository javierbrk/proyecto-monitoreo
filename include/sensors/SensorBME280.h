#ifndef SENSOR_BME280_H
#define SENSOR_BME280_H

#include "ISensor.h"
#include "ITemperatureSensor.h"
#include "IHumiditySensor.h"
#include "IPressureSensor.h"
#include <Adafruit_BME280.h>
#include "../debug.h"

class SensorBME280 : public ITemperatureSensor, public IHumiditySensor, public IPressureSensor {
private:
    Adafruit_BME280 bme;
    bool active;
    float temperature;
    float humidity;
    float pressure;
    uint8_t address = 0x76;

public:
    SensorBME280() : active(false), temperature(999), humidity(100), pressure(0) {}

    bool init() override {
        // Try I2C address 0x76 (default) or 0x77 (alternate)
        active = bme.begin(0x76) || (bme.begin(0x77) && ((address = 0x77) || true));

        if (!active) {
            DBG_ERROR("[BME280] Init failed\n");
        } else {
            DBG_INFO("[BME280] OK\n");
            bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                          Adafruit_BME280::SAMPLING_X2,
                          Adafruit_BME280::SAMPLING_X16,
                          Adafruit_BME280::SAMPLING_X1,
                          Adafruit_BME280::FILTER_X16,
                          Adafruit_BME280::STANDBY_MS_500);
        }
        return active;
    }

    bool dataReady() override {
        return active;
    }

    bool read() override {
        if (!active) return false;

        temperature = bme.readTemperature();
        humidity = bme.readHumidity();
        pressure = bme.readPressure() / 100.0F;  // Convert Pa to hPa

        if (isnan(temperature) || isnan(humidity) || isnan(pressure)) {
            DBG_ERROR("[BME280] Read error\n");
            return false;
        }

        return true;
    }

    // ITemperatureSensor
    float getTemperature() override { return temperature; }

    // IHumiditySensor
    float getHumidity() override { return humidity; }

    // IPressureSensor
    float getPressure() override { return pressure; }

    const char* getSensorType() override { return "BME280"; }

    const char* getSensorID() override {
        static char idString[16];
        snprintf(idString, sizeof(idString), "thp-i2c-%u", (unsigned)address);
        return idString;
    }

    const char* getMeasurementsString() override {
        static char measString[64];
        snprintf(measString, sizeof(measString), "temp=%.1f,hum=%.1f,press=%.1f", temperature, humidity, pressure);
        return measString;
    }

    bool isActive() override { return active; }
};

#endif // SENSOR_BME280_H
