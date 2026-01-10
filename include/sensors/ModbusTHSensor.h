#ifndef MODBUS_TH_SENSOR_H
#define MODBUS_TH_SENSOR_H

#include "ModbusSensorBase.h"
#include "ITemperatureSensor.h"
#include "IHumiditySensor.h"
#include "../debug.h"

/**
 * Sensor TH-MB-04S - Temperature & Humidity via Modbus RTU
 *
 * Protocol: Modbus RTU over RS485
 * Registers:
 *   - 0 (0x00): Humidity (int16 * 10, e.g., 399 = 39.9% RH)
 *   - 1 (0x01): Temperature (int16 * 10, e.g., 282 = 28.2°C)
 */
class ModbusTHSensor : public ModbusSensorBase<2>, public ITemperatureSensor, public IHumiditySensor {
private:
    float temperature = 999;
    float humidity = 999;

protected:
    const char* getLogPrefix() const override {
        return "ModbusTH";
    }

    void parseRegisters() override {
        humidity = registerBuffer[0] / 10.0;
        temperature = registerBuffer[1] / 10.0;

        DBG_VERBOSE("[ModbusTH] %d: T=%.1fC H=%.1f%%\n",
                     modbusAddress, temperature, humidity);
    }

    void setInvalidValues() override {
        humidity = 99;
        temperature = 999;
    }

public:
    ModbusTHSensor(uint8_t address = 1)
        : ModbusSensorBase<2>(address),
          temperature(999),
          humidity(99) {
    }

    // ITemperatureSensor
    float getTemperature() override { return temperature; }

    // IHumiditySensor
    float getHumidity() override { return humidity; }

    const char* getSensorType() override {
        static char sensorName[32];
        snprintf(sensorName, sizeof(sensorName), "modbus_th_%d", modbusAddress);
        return sensorName;
    }

    const char* getSensorID() override {
        static char sensorID[32];
        snprintf(sensorID, sizeof(sensorID), "th-mod-%d", modbusAddress);
        return sensorID;
    }

    const char* getMeasurementsString() override {
        static char measString[32];
        snprintf(measString, sizeof(measString), "temp=%.1f,hum=%.1f", temperature, humidity);
        return measString;
    }
};

#endif // MODBUS_TH_SENSOR_H
