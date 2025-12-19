#ifndef ISENSOR_H
#define ISENSOR_H

/**
 * Base interface for all sensors.
 *
 * Contains only lifecycle and identification methods.
 * Measurement capabilities are provided by extended interfaces:
 *   - ITemperatureSensor: getTemperature()
 *   - IHumiditySensor: getHumidity() (air)
 *   - IMoistureSensor: getMoisture() (soil)
 *   - ICO2Sensor: getCO2()
 *   - IPressureSensor: getPressure()
 *   - ISoilSensor: getEC(), getPH(), getNitrogen(), getPhosphorus(), getPotassium()
 *
 * Use dynamic_cast to check sensor capabilities:
 *   auto* temp = dynamic_cast<ITemperatureSensor*>(sensor);
 *   if (temp) { float t = temp->getTemperature(); }
 */
class ISensor {
public:
    virtual ~ISensor() = default;

    // Lifecycle
    virtual bool init() = 0;
    virtual bool dataReady() = 0;
    virtual bool read() = 0;
    virtual bool isActive() = 0;

    // Identification
    virtual const char* getSensorType() = 0;
    virtual const char* getSensorID() = 0;

    // All measurements as string (for logging/display)
    virtual const char* getMeasurementsString() = 0;

    // Optional calibration
    virtual bool calibrate(float reference = 0) { return false; }
};

#endif // ISENSOR_H
