# API Reference

Endpoints HTTP del sistema de monitoreo.

**Base URL:** `http://{IP_DEL_ESP32}`

**Puerto:** 80

## Endpoints

### GET /actual

Lecturas actuales de sensores.

**Response (ejemplo real):**
```json
{
  "rotation": false,
  "a_pressure": "99.00",
  "a_temperature": "99.00",
  "a_humidity": "100.00",
  "a_co2": "999999.00",
  "wifi_status": "connected",
  "errors": {
    "rotation": [],
    "temperature": [],
    "sensors": [],
    "humidity": [],
    "wifi": []
  }
}
```

**Códigos:**
- `200`: OK
- Valores default si sensor inactivo: temp=99, hum=100, co2=999999, pressure=99

---

### GET /data

Vista HTML de datos de sensores.

**Response:** HTML con tabla de datos
- Auto-refresh cada 10s (meta refresh)
- Incluye temperatura, humedad, CO2

---

### GET /config

Configuración actual del sistema.

**Response (ejemplo real):** JSON con contenido completo de config.json

```json
{
  "max_temperature": 37.7,
  "min_temperature": 37.3,
  "rotation_duration": 50000,
  "rotation_period": 3600000,
  "ssid": "ToChange",
  "passwd": "ToChange",
  "incubation_period": 18,
  "max_hum": 65,
  "min_hum": 55,
  "hash": "80F3DAACC7D4",
  "incubator_name": "moni-80F3DAACC7D4",
  "sensors": [
    {"type": "scd30", "enabled": false, "config": {}},
    {"type": "bme280", "enabled": false, "config": {}},
    {"type": "modbus_th", "enabled": false, "config": {"addresses": [2,3,4,5,6,7,8,9,10,45]}},
    {"type": "onewire", "enabled": true, "config": {"pin": 14, "scan": true}},
    {"type": "capacitive", "enabled": false, "config": {"pin": 34, "name": "Soil1"}},
    {"type": "hd38", "enabled": true, "config": {"voltage_divider": false, "invert_logic": false, "analog_pins": [35, 32]}}
  ],
  "espnow_enabled": false,
  "espnow_force_mode": "gateway",
  "espnow_channel": 11,
  "beacon_interval_ms": 2000,
  "discovery_timeout_ms": 15000,
  "send_interval_ms": 30000,
  "grafana_ping_url": "",
  "current_wifi_channel": 11,
  "relays": [{"type": "relay_2ch", "enabled": false, "config": {"alias": "Relay 01", "address": 1}}],
  "rs485": {"enabled": false, "rx_pin": 16, "tx_pin": 17, "de_pin": 18, "baudrate": 9600, "raw_send_enabled": false}
}
```

**Códigos:**
- `200`: OK
- `404`: Config file no encontrado (retorna `{}`)

---

### POST /config

Actualizar configuración.

**Request:**
```
Content-Type: application/json

{config.json completo o parcial}
```

**Comportamiento:**
- Merge con config existente
- Guarda a SPIFFS (`/config.json`)
- Si SSID cambió: llama `wifiManager.onChange()`
  - WiFi intenta nueva conexión
  - Rollback a credenciales anteriores si falla

**Response:**
```json
{"status": "success"}
```

**Códigos:**
- `200`: Config guardada
- `400`: JSON inválido
- `500`: Error escribiendo SPIFFS

**Require restart para:**
- Cambios en sensores
- Cambios en ESP-NOW mode
- Cambios en pines RS485

---

### POST /config/reset

Reset a configuración default.

**Request:** (vacío o cualquier body)

**Comportamiento:**
1. Borra `/config.json` de SPIFFS
2. Crea nueva config con defaults
3. Guarda a SPIFFS
4. Reinicia ESP32 (delay 1s)

**Response:**
```json
{"status": "reset", "message": "Configuración reseteada. Reiniciando..."}
```

**Códigos:**
- `200`: OK, reiniciando

**Defaults creados:**
- SSID: "ToChange"
- Password: "ToChange"
- Nombre: "moni-{MAC}"
- RS485: Disabled
- ESP-NOW: Disabled
- Sensors: SCD30 enabled, resto disabled

---

### GET /calibrate-scd30

Calibra sensor SCD30 a 400ppm.

**Comportamiento:**

- Solo funciona con sensor SCD30
- Llama `sensor->calibrate(400.0)`
- Requiere sensor activo e inicializado

**Response (sin sensor activo - ejemplo real):**

```json
{
  "status": "error",
  "message": "No sensor active",
  "sensor_detected": false,
  "calibration_performed": false
}
```

**Response (con sensor SCD30 activo):**

```json
{"status": "success", "message": "SCD30 calibrado a 400ppm"}
```

**Códigos:**

- `200`: Calibración ejecutada o error informado

**Recomendación:** Ejecutar con sensor en ambiente 400ppm CO2 (aire exterior)

---

### GET /settings

Formulario web de configuración.

**Response (ejemplo real):** HTML con form completo

```html
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="icon" type="image/svg+xml" href="/favicon.svg">
    <title>Configuración - Monitor</title>
    ...
</head>
```

**Contenido:**

- Inputs para WiFi (SSID, password)
- Selección de sensores (SCD30, BME280, OneWire, etc.)
- Config RS485 (pines, baudrate)
- Config ESP-NOW (modo, canal)
- Botones de gestión

**Funciones del form:**

- Submit POST a `/config`
- Botón "Restart" → POST a `/restart`
- Botón "Load Default Sensors" → carga config predefinida
- Muestra estado actual de ESP-NOW en tiempo real

**Códigos:**

- `200`: OK

---

### POST /restart

Reinicia el ESP32.

**Request:** (vacío o cualquier body)

**Comportamiento:**
- Delay 1000ms
- Llama `ESP.restart()`

**Response:**
```json
{"status": "restarting"}
```

**Códigos:**
- `200`: OK, reiniciando

**Nota:** Conexión HTTP se cortará durante restart

---

### GET /espnow/status

Estado actual de ESP-NOW.

**Response (ejemplo real):**

```json
{
  "enabled": false,
  "mode": "sensor",
  "forced_mode": "gateway",
  "mac_address": "80:F3:DA:AC:C7:D4",
  "channel": 11,
  "paired": false,
  "peer_count": 0
}
```

**Campos:**

- `enabled`: ESP-NOW habilitado en config
- `mode`: "gateway", "sensor", o "disabled"
- `forced_mode`: Modo forzado en config (si está vacío, usa auto-detección)
- `mac_address`: Dirección MAC del dispositivo
- `channel`: Canal WiFi actual
- `paired`: (solo sensor) Si encontró y se pareó con gateway
- `peer_count`: (solo gateway) Número de sensores pareados

**Códigos:**

- `200`: OK
- Si ESP-NOW disabled: `enabled: false`, resto con valores default

---

### GET /api/relays

Lista de relays configurados.

**Response (ejemplo real):**

```json
[]
```

**Response (con relays activos):**

```json
[
  {
    "address": 1,
    "alias": "Relay 01",
    "channels": 2,
    "states": [false, false]
  }
]
```

**Campos:**

- `address`: Dirección Modbus del relay
- `alias`: Nombre descriptivo
- `channels`: Número de canales
- `states`: Array con estado de cada canal (true=ON, false=OFF)

**Códigos:**

- `200`: OK (array vacío si no hay relays configurados)

---

### POST /api/relay/toggle

Cambia estado de un canal de relay.

**Query params:**

- `addr`: Dirección del relay (requerido)
- `ch`: Número de canal (requerido, 0-indexed)

**Ejemplo:**

```bash
curl -X POST "http://10.128.184.25/api/relay/toggle?addr=1&ch=0"
```

**Response:**

```json
{"status": "ok", "address": 1, "channel": 0, "new_state": true}
```

**Códigos:**

- `200`: Toggle exitoso
- `400`: Parámetros faltantes o inválidos
- `404`: Relay no encontrado

---

### GET /favicon.svg

Ícono del sitio.

**Response (si existe):** SVG con logo AlterMundi

- Served from SPIFFS `/favicon.svg`
- Content-Type: image/svg+xml

**Response (ejemplo real - archivo no existe):**

```http
HTTP/1.1 302 Found
Location: /
```

**Códigos:**

- `200`: OK (archivo existe)
- `302`: Redirect a `/` (archivo no existe en SPIFFS)

---

### GET /* (404 handler)

Cualquier ruta no definida.

**Comportamiento:**
- Redirect 302 a `/`

**Response:**
```
Location: /
```

**Códigos:**
- `302`: Found (redirect)

---

## Autenticación

Ningún endpoint requiere autenticación.

**Seguridad:**
- Sistema asume red local confiable
- WiFi AP del ESP32 sin password (captive portal)
- Considerar restricciones de firewall si se expone a internet

---

## Rate Limiting

No hay rate limiting implementado.

**Consideraciones:**
- Llamadas HTTP bloqueantes en main loop
- Request intensivo puede afectar lectura de sensores
- Recomendado: Max 1 req/s por endpoint

---

## CORS

No hay headers CORS.

**Workaround:** Usar desde mismo origen o proxy CORS

---

## Ejemplos de Uso

### Leer sensor actual
```bash
curl http://192.168.1.100/actual
```

### Cambiar SSID WiFi
```bash
curl -X POST http://192.168.1.100/config \
  -H "Content-Type: application/json" \
  -d '{"ssid":"NewWiFi","passwd":"newpass"}'
```

### Habilitar RS485
```bash
curl -X POST http://192.168.1.100/config \
  -H "Content-Type: application/json" \
  -d '{"rs485_enabled":true}'

# Restart required
curl -X POST http://192.168.1.100/restart
```

### Habilitar sensor OneWire
```bash
curl -X POST http://192.168.1.100/config \
  -H "Content-Type: application/json" \
  -d '{
    "sensors": [
      {
        "type": "onewire",
        "enabled": true,
        "config": {"pin": 4, "scan": true}
      }
    ]
  }'
```

### Check ESP-NOW status
```bash
curl http://192.168.1.100/espnow/status | jq
```

### Factory reset
```bash
curl -X POST http://192.168.1.100/config/reset
# Dispositivo reiniciará con defaults
```

---

## Errores Comunes

### JSON parse error (POST /config)
**Síntoma:** Config no se guarda
**Causa:** JSON malformado
**Fix:** Validar JSON antes de enviar (jq, jsonlint)

### Config changes no surten efecto
**Síntoma:** Cambios guardados pero sin efecto
**Causa:** Require restart
**Fix:** POST a `/restart` después de cambiar sensors/espnow/rs485

### Timeout en requests
**Síntoma:** curl timeout
**Causa:** Main loop bloqueado (envío a Grafana)
**Fix:** Esperar ~5s y reintentar

### WiFi credential change rompe conexión
**Síntoma:** No se puede acceder después de cambiar SSID
**Causa:** Nuevas credenciales incorrectas, ESP32 intentó conectar
**Fix:**
1. Esperar 30s (timeout validation)
2. Rollback automático a credenciales viejas
3. O conectarse al AP del ESP32 y reconfigurar
