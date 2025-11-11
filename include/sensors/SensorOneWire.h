#ifndef SENSOR_ONEWIRE_H
#define SENSOR_ONEWIRE_H

#include "ISensor.h"
#include <OneWire.h>
#include <DallasTemperature.h>

class SensorOneWire : public ISensor {
private:
    DallasTemperature* dallas;
    DeviceAddress address;  // 64-bit unique address
    String addressStr;      // Hex string for identification
    float temperature;
    int deviceIndex;
    bool active;

public:
    SensorOneWire(DallasTemperature* dt, DeviceAddress addr, int idx)
        : dallas(dt), deviceIndex(idx), temperature(-127), active(false) {
        memcpy(address, addr, 8);

        // Convert address to hex string for identification
        addressStr = "";
        for (uint8_t i = 0; i < 8; i++) {
            if (address[i] < 16) addressStr += "0";
            addressStr += String(address[i], HEX);
        }
        addressStr.toUpperCase();
    }

    bool init() override {
        if (dallas) {
            dallas->setResolution(address, 12);  // 12-bit resolution
            active = true;
            Serial.printf("OneWire sensor %s inicializado\n", addressStr.c_str());
            return true;
        }
        return false;
    }

    bool dataReady() override {
        return active;
    }

    bool read() override {
        if (!active || !dallas) return false;

        // Request temperature (non-blocking after first call)
        float temp = dallas->getTempC(address);

        if (temp != DEVICE_DISCONNECTED_C && temp != 85.0) {  // 85.0 = not ready
            temperature = temp;
            return true;
        }

        return false;
    }

    float getTemperature() override { return temperature; }
    float getHumidity() override { return -1; }  // Not available
    float getCO2() override { return -1; }       // Not available

    const char* getSensorType() override { return "OneWire"; }

    bool isActive() override { return active; }

    // OneWire-specific methods
    String getAddress() const { return addressStr; }

    String getSensorId() const {
        return "OneWire_" + addressStr.substring(0, 12);  // First 12 chars for brevity
    }
};

#endif // SENSOR_ONEWIRE_H
