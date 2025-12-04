#ifndef MODBUS_TH_SENSOR_H
#define MODBUS_TH_SENSOR_H

#include "ISensor.h"
#include <ModbusRTU.h>
#include <HardwareSerial.h>

/**
 * Sensor TH-MB-04S - Temperatura y Humedad via Modbus RTU
 *
 * Protocolo: Modbus RTU sobre RS485
 * Registros:
 *   - 0: Humedad (int16 * 10, ej: 556 = 55.6% RH)
 *   - 1: Temperatura (int16 * 10, ej: 259 = 25.9°C)
 *
 * Config:
 *   - address: Dirección Modbus (1-254)
 *   - rxPin: GPIO RX RS485 (default 16)
 *   - txPin: GPIO TX RS485 (default 17)
 *   - dePin: GPIO DE/RE control (optional, -1 if puenteado)
 *   - baudrate: Baudrate RS485 (default 9600)
 */
class ModbusTHSensor : public ISensor {
private:
    ModbusRTU mb;
    HardwareSerial* serial;

    uint8_t modbusAddress;
    int rxPin;
    int txPin;
    int dePin;
    uint32_t baudrate;

    float temperature;
    float humidity;
    bool active;

    // Modbus read result callbacks
    static uint16_t registerBuffer[2];  // Buffer para humedad y temperatura
    static bool readComplete;

    static bool readCallback(Modbus::ResultCode event, uint16_t transactionId, void* data) {
        if (event == Modbus::EX_SUCCESS) {
            readComplete = true;
            return true;
        } else {
            Serial.printf("[ModbusTH] Read error: %02X\n", event);
            readComplete = false;
            return false;
        }
    }

public:
    ModbusTHSensor(uint8_t address = 1,
                   int rx = 16,
                   int tx = 17,
                   int de = -1,
                   uint32_t baud = 9600)
        : modbusAddress(address),
          rxPin(rx),
          txPin(tx),
          dePin(de),
          baudrate(baud),
          temperature(-1),
          humidity(-1),
          active(false) {

        serial = &Serial2;
    }

    bool init() override {
        Serial.printf("[ModbusTH] Initializing addr=%d, RX=%d, TX=%d, DE=%d, baud=%d\n",
                      modbusAddress, rxPin, txPin, dePin, baudrate);

        // Init serial port
        serial->begin(baudrate, SERIAL_8N1, rxPin, txPin);

        // Init Modbus RTU
        mb.begin(serial);
        mb.master();

        // Configure DE/RE pin if provided
        if (dePin >= 0) {
            pinMode(dePin, OUTPUT);
            digitalWrite(dePin, LOW);  // Receive mode by default
        }

        // Test read to verify sensor is accessible
        delay(100);
        bool testRead = readRegisters();

        if (testRead) {
            active = true;
            Serial.println("[ModbusTH] Initialized successfully");
        } else {
            active = false;
            Serial.println("[ModbusTH] Initialization failed - sensor not responding");
        }

        return active;
    }

    bool dataReady() override {
        return active;
    }

    bool read() override {
        if (!active) return false;

        bool success = readRegisters();

        if (success) {
            // Registers come multiplied by 10
            humidity = registerBuffer[0] / 10.0;
            temperature = registerBuffer[1] / 10.0;

            Serial.printf("[ModbusTH] Addr %d: T=%.1f°C, H=%.1f%%\n",
                         modbusAddress, temperature, humidity);
            return true;
        }

        return false;
    }

    float getTemperature() override {
        return temperature;
    }

    float getHumidity() override {
        return humidity;
    }

    float getCO2() override {
        return -1;  // Not supported
    }

    const char* getSensorType() override {
        static char sensorName[32];
        snprintf(sensorName, sizeof(sensorName), "modbus_th_%d", modbusAddress);
        return sensorName;
    }

    bool calibrate(float reference) override {
        // TH-MB-04S supports calibration via registers 80/81
        // Not implemented yet - requires write to holding registers
        return false;
    }

    bool isActive() override {
        return active;
    }

private:
    bool readRegisters() {
        readComplete = false;

        // Read 2 holding registers starting from address 0
        // Register 0: Humidity
        // Register 1: Temperature
        if (!mb.readHreg(modbusAddress, 0, registerBuffer, 2, readCallback)) {
            Serial.println("[ModbusTH] Failed to initiate read request");
            return false;
        }

        // Wait for response (with timeout)
        unsigned long startTime = millis();
        while (!readComplete && (millis() - startTime < 1000)) {
            mb.task();  // Process Modbus events
            delay(10);
        }

        if (!readComplete) {
            Serial.println("[ModbusTH] Read timeout");
            return false;
        }

        return true;
    }
};

// Static member initialization
uint16_t ModbusTHSensor::registerBuffer[2] = {0, 0};
bool ModbusTHSensor::readComplete = false;

#endif // MODBUS_TH_SENSOR_H
