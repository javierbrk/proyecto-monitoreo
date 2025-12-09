#ifndef ISENSOR_H
#define ISENSOR_H

class ISensor {
public:
    virtual ~ISensor() {}

    // Inicialización del sensor
    virtual bool init() = 0;

    // Verificar si hay datos disponibles para leer
    virtual bool dataReady() = 0;

    // Leer datos del sensor
    virtual bool read() = 0;

    // Getters para valores medidos
    virtual float getTemperature() = 0;  // Celsius
    virtual float getHumidity() = 0;     // Percentage 0-100
    virtual float getCO2() = 0;          // ppm (-1 si no aplica)

    virtual float getPressure() { return -1; } // hPa (-1 si no aplica)
    virtual const char* getMeasurementsString() = 0; // string with all measurements
    // Nombre del tipo de sensor
    virtual const char* getSensorType() = 0;

    // Identificador único del sensor
    /*
    type-interface-id
    type:
    temp: t
    hum: h
    temp&hum: th
    temp&hum&pressure: thp
    co2: c

    interface:
    modbus:mo
    1w
    i2c
    adc

    in device id:
    modbus: "address"
    1w: "mac (las 4 bytes)"
    i2c: "address"
    adc: "pin*/
    virtual const char* getSensorID() = 0;

    // Métodos opcionales
    virtual bool calibrate(float reference = 0) { return false; }
    virtual bool isActive() = 0;
};

#endif // ISENSOR_H
