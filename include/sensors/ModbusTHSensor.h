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
 *   - 1 (0x01): Humedad (int16 * 10, ej: 399 = 39.9% RH)
 *   - 2 (0x02): Temperatura (int16 * 10, ej: 282 = 28.2°C)
 *
 * Soporta multiples sensores en el mismo bus RS485 con diferentes direcciones.
 * El bus se inicializa una sola vez y se comparte entre todas las instancias.
 *
 * Config:
 *   - address: Direccion Modbus (1-254)
 *   - rxPin: GPIO RX RS485 (default 16)
 *   - txPin: GPIO TX RS485 (default 17)
 *   - dePin: GPIO DE/RE control (default 18, -1 si no usa)
 *   - baudrate: Baudrate RS485 (default 9600)
 */
class ModbusTHSensor : public ISensor {
private:
    // Shared bus resources (static)
    static ModbusRTU* sharedMb;
    static HardwareSerial* sharedSerial;
    static bool busInitialized;
    static int busRxPin;
    static int busTxPin;
    static int busDePin;
    static uint32_t busBaudrate;

    uint8_t modbusAddress;
    int rxPin;
    int txPin;
    int dePin;
    uint32_t baudrate;

    float temperature = 999;
    float humidity = 99;
    bool active;

    // Modbus read result callbacks
    static uint16_t registerBuffer[2];
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

    // Initialize shared bus (only once)
    static bool initBus(int rx, int tx, int de, uint32_t baud) {
        if (busInitialized) {
            // Check if config matches
            if (rx != busRxPin || tx != busTxPin || de != busDePin || baud != busBaudrate) {
                Serial.println("[ModbusTH] Warning: Bus config mismatch, using existing config");
            }
            return true;
        }

        Serial.printf("[ModbusTH] Initializing shared bus: RX=%d, TX=%d, DE=%d, baud=%d\n",
                      rx, tx, de, baud);

        sharedSerial = &Serial2;
        sharedSerial->begin(baud, SERIAL_8N1, rx, tx);

        sharedMb = new ModbusRTU();
        sharedMb->begin(sharedSerial, de);
        sharedMb->master();

        busRxPin = rx;
        busTxPin = tx;
        busDePin = de;
        busBaudrate = baud;
        busInitialized = true;

        return true;
    }

public:
    ModbusTHSensor(uint8_t address = 1,
                   int rx = 16,
                   int tx = 17,
                   int de = 18,
                   uint32_t baud = 9600)
        : modbusAddress(address),
          rxPin(rx),
          txPin(tx),
          dePin(de),
          baudrate(baud),
          temperature(-1),
          humidity(-1),
          active(false) {
    }

    bool init() override {
        Serial.printf("[ModbusTH] Initializing sensor addr=%d\n", modbusAddress);

        // Initialize shared bus
        if (!initBus(rxPin, txPin, dePin, baudrate)) {
            Serial.println("[ModbusTH] Failed to initialize bus");
            return false;
        }

        // Test read to verify sensor is accessible
        delay(100);
        bool testRead = readRegisters();

        if (testRead) {
            active = true;
            Serial.printf("[ModbusTH] Sensor addr=%d initialized successfully\n", modbusAddress);
        } else {
            active = false;
            Serial.printf("[ModbusTH] Sensor addr=%d not responding\n", modbusAddress);
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

            Serial.printf("[ModbusTH] Addr %d: T=%.1f C, H=%.1f%%\n",
                         modbusAddress, temperature, humidity);
            return true;
        }
        // On failure, set invalid values ... do not just keep the lastone 
        humidity = 99;
        temperature = 999;

        Serial.printf("[ModbusTH] Addr %d: T=%.1f C, H=%.1f%%\n",
                        modbusAddress, temperature, humidity);
        Serial.printf("[ModbusTH] Addr %d: Read failed\n", modbusAddress);
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
    // Identifier format: "th-mod-address"
    const char* getSensorID() override {
        static char sensorID[32];
        snprintf(sensorID, sizeof(sensorID), "th-mod-%d", modbusAddress);
        return sensorID;
    }

    const char* getMeasurementsString() override {
        static char measString[32];
        //" temp=" + String(temperature, 2) + ",hum=" + String(humidity, 2) +  ",co2=" + String(co2) +
        snprintf(measString, sizeof(measString), "temp=%.1f,hum=%.1f", temperature, humidity);
        return measString;
    }

    bool calibrate(float reference) override {
        return false;
    }

    bool isActive() override {
        return active;
    }

    uint8_t getAddress() const {
        return modbusAddress;
    }

private:
    bool readRegisters() {
        if (!sharedMb) return false;

        readComplete = false;

        // Read 2 holding registers starting from address 1
        // Register 0 (0x00): Humidity
        // Register 1 (0x01): Temperature
        if (!sharedMb->readHreg(modbusAddress, 0, registerBuffer, 2, readCallback)) {
            Serial.printf("[ModbusTH] Addr %d: Failed to initiate read\n", modbusAddress);
            return false;
        }

        // Wait for response (with timeout)
        unsigned long startTime = millis();
        while (!readComplete && (millis() - startTime < 1000)) {
            sharedMb->task();
            delay(10);
        }

        if (!readComplete) {
            Serial.printf("[ModbusTH] Addr %d: Read timeout\n", modbusAddress);
            return false;
        }

        return true;
    }
};

// Static member initialization
ModbusRTU* ModbusTHSensor::sharedMb = nullptr;
HardwareSerial* ModbusTHSensor::sharedSerial = nullptr;
bool ModbusTHSensor::busInitialized = false;
int ModbusTHSensor::busRxPin = -1;
int ModbusTHSensor::busTxPin = -1;
int ModbusTHSensor::busDePin = -1;
uint32_t ModbusTHSensor::busBaudrate = 0;
uint16_t ModbusTHSensor::registerBuffer[2] = {0, 0};
bool ModbusTHSensor::readComplete = false;

#endif // MODBUS_TH_SENSOR_H
