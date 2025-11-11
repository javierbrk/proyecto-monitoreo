# Proyecto Monitoreo - Multi-Sensor + RS485

Fork of [AlterMundi-MonitoreoyControl/proyecto-monitoreo](https://github.com/AlterMundi-MonitoreoyControl/proyecto-monitoreo) with RS485 connectivity and multi-sensor support.

Branch `485_LoRa_Now` adds:
- RS485 communication for remote sensor networks
- Multi-sensor architecture supporting simultaneous sensors
- OneWire support for DS18B20 temperature sensor chains
- Runtime configuration via `config.json`

All active sensors propagate data to **both** Grafana (InfluxDB) and RS485 outputs.

## Supported Sensors

| Sensor | Type | Protocol | Measured |
|--------|------|----------|----------|
| SCD30 | CO2/Temp/Humidity | I2C | CO2, Temperature, Humidity |
| BME280 | Temp/Humidity/Pressure | I2C | Temperature, Humidity |
| Capacitive | Soil Moisture | ADC | Humidity (soil) |
| DS18B20 | Temperature | OneWire | Temperature |

## Build Environments

```bash
# Single sensor builds (compile-time selection)
pio run -e esp32dev                # SCD30 only
pio run -e esp32dev_capacitive     # Capacitive soil sensor
pio run -e esp32dev_bme280         # BME280 only
pio run -e esp32dev_rs485          # Capacitive + RS485

# Multi-sensor build (runtime config via config.json)
pio run -e esp32dev_multi          # All sensors + RS485
```

### Multi-Sensor Mode

Enabled with `-DSENSOR_MULTI` build flag. Uses `SensorManager` to handle multiple simultaneous sensors configured via `config.json`.

**Features:**
- Auto-detection of OneWire sensors on bus
- Individual sensor enable/disable without recompilation
- Unique sensor IDs for Grafana tagging
- Shared DallasTemperature instances for efficiency

## Configuration

### config.json Structure

```json
{
  "sensors": [
    {
      "type": "capacitive",
      "enabled": true,
      "config": {
        "pin": 34,
        "name": "Soil1"
      }
    },
    {
      "type": "scd30",
      "enabled": true,
      "config": {}
    },
    {
      "type": "onewire",
      "enabled": true,
      "config": {
        "pin": 4,
        "scan": true
      }
    }
  ],
  "rs485_enabled": true,
  "rs485_rx": 16,
  "rs485_tx": 17,
  "rs485_baud": 9600
}
```

### Sensor Types

#### SCD30 (I2C)
```json
{"type": "scd30", "enabled": true, "config": {}}
```

#### Capacitive Soil Moisture
```json
{
  "type": "capacitive",
  "enabled": true,
  "config": {"pin": 34}
}
```

#### BME280 (I2C)
```json
{"type": "bme280", "enabled": true, "config": {}}
```

#### OneWire (DS18B20 chain)
```json
{
  "type": "onewire",
  "enabled": true,
  "config": {
    "pin": 4,
    "scan": true
  }
}
```

When `scan: true`, automatically detects all DS18B20 sensors on the bus and creates individual sensor instances with unique IDs.

## Hardware Setup

### SCD30 Wiring
```
ESP32        SCD30
D22    <->   SCL
D21    <->   SDA
3V3    <->   Vin
GND    <->   GND
```

### Capacitive Soil Sensor
```
ESP32        Sensor
GPIO34 <->   AOUT
3V3    <->   VCC
GND    <->   GND
```

### BME280 Wiring
```
ESP32        BME280
D22    <->   SCL
D21    <->   SDA
3V3    <->   VCC
GND    <->   GND
```

### DS18B20 OneWire Chain
```
ESP32        DS18B20(s)
GPIO4  <->   DQ (data)
3V3    <->   VDD
GND    <->   GND

Note: Requires 4.7kΩ pull-up resistor between DQ and VDD
Multiple sensors share same bus (parallel wiring)
```

### RS485 Connection

**Puenteado Mode (TX-only, recommended for simple setups):**
```
ESP32        MAX485/Similar
GPIO17 <->   DI (data in)
GPIO16       (not connected)
3V3    <->   VCC
GND    <->   GND

On RS485 module:
- Bridge DE and RE pins together to VCC (always transmit)
- A/B terminals to RS485 bus
```

**Full Control Mode (with DE/RE pins):**
```
ESP32        MAX485
GPIO17 <->   DI
GPIO16 <->   RO
GPIO18 <->   DE
GPIO19 <->   RE
3V3    <->   VCC
GND    <->   GND
```

Configure in `config.json`:
```json
{
  "rs485_rx": 16,
  "rs485_tx": 17,
  "rs485_baud": 9600
}
```

## Data Flow

```
Sensors → SensorManager.readAll()
           ↓
    ┌──────┴──────┐
    ↓             ↓
Grafana        RS485
(InfluxDB)     (Serial)
```

Each sensor reading is sent to **both** outputs with sensor-specific tags/identifiers.

### Grafana Format (InfluxDB Line Protocol)
```
sensor,host=<hash>,type=<sensor_id> temperature=<t>,humidity=<h>,co2=<c>
```

### RS485 Format
```
<SensorType> - Temp: <t>°C Humedad: <h>% CO2: <c>ppm\r\n
```

## Web Configuration Interface

Access the configuration interface at `http://<ESP32_IP>/settings`

**Features:**
- View and modify all configuration parameters without recompilation
- Enable/disable individual sensors
- Configure RS485 settings (pins, baudrate)
- Update WiFi credentials
- Adjust temperature and humidity thresholds
- Restart device remotely

**Usage:**
1. Connect to ESP32 WiFi network or ensure device is on local network
2. Navigate to `http://<device_ip>/settings`
3. Modify desired parameters
4. Click "Guardar Configuración"
5. Restart device if prompted for changes to take effect

**API Endpoints:**
- `GET /settings` - Configuration form UI
- `GET /config` - Current configuration (JSON)
- `POST /config` - Update configuration (JSON body)
- `POST /restart` - Restart ESP32
- `GET /data` - Sensor readings (HTML table)
- `GET /actual` - Sensor readings (JSON)
- `GET /calibrate-scd30` - Calibrate SCD30 to 400ppm

## Troubleshooting

### RS485 No Output

1. **Check build environment**
   ```bash
   # Must use rs485 or multi environment
   pio run -e esp32dev_multi --target upload
   ```

2. **Verify config.json**
   ```json
   {"rs485_enabled": true}
   ```

3. **Monitor serial output** for initialization:
   ```
   RS485 inicializado sin control DE/RE (puenteado)
   RS485: RX=16, TX=17, Baud=9600
   ```

4. **Check wiring**
   - TX pin (GPIO17) connected to RS485 DI
   - DE/RE bridged to VCC for puenteado mode
   - Correct baud rate on receiving device

5. **Verify data transmission** on Serial Monitor:
   ```
   [RS485 TX] Capacitive - Humedad: 65.2%
   ```

### OneWire Sensors Not Detected

1. **Check pull-up resistor** (4.7kΩ between DQ and VDD)
2. **Verify sensor power** (3.3V to VDD, not 5V)
3. **Enable scanning** in config.json: `"scan": true`
4. **Monitor serial output**:
   ```
   OneWire: 3 sensors detected on pin 4
   ```

### I2C Sensors Not Found

1. **Check wiring** (SDA/SCL, VCC/GND)
2. **Verify I2C address** with scanner sketch
3. **Check power supply** (stable 3.3V)
4. **Monitor serial output**:
   ```
   No se pudo inicializar el sensor SCD30!
   ```

## Memory Usage

Typical for `esp32dev_multi` with all sensors:
- **RAM**: ~49KB (15%)
- **Flash**: ~1.1MB (85%)

## Development

### Adding New Sensor Type

1. Create header in `include/sensors/` implementing `ISensor` interface
2. Add to `SensorManager::loadFromConfig()` switch statement
3. Update `platformio.ini` lib_deps if needed
4. Add config.json example to README

### Testing

```bash
# Build and upload
pio run -e esp32dev_multi --target upload

# Monitor output
pio device monitor -e esp32dev_multi

# Check RS485 output (Linux)
screen /dev/ttyUSB1 9600
```

## License

Same as upstream AlterMundi-MonitoreoyControl/proyecto-monitoreo
