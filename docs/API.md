# API Reference

Endpoints HTTP del sistema de monitoreo.

**Base URL:** `http://{IP_DEL_ESP32}`

**Puerto:** 80

## Endpoints

### GET /actual

Lecturas actuales de sensores.

**Response:**
```json
{
  "rotation": false,
  "a_pressure": "99.00",
  "a_temperature": "25.30",
  "a_humidity": "60.50",
  "a_co2": "450.00",
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
- Valores default si sensor inactivo: temp=99, hum=100, co2=999999

---

### GET /data

Vista HTML de datos de sensores.

**Response:** HTML con tabla de datos
- Auto-refresh cada 10s (meta refresh)
- Incluye temperatura, humedad, CO2

---

### GET /config

Configuración actual del sistema.

**Response:** JSON con contenido completo de config.json
```json
{
  "ssid": "MyWiFi",
  "passwd": "password",
  "incubator_name": "moni-AABBCCDDEE",
  "hash": "AABBCCDDEE",
  "min_temperature": 37.3,
  "max_temperature": 37.7,
  "min_hum": 55,
  "max_hum": 65,
  "rs485_enabled": false,
  "rs485_rx": 16,
  "rs485_tx": 17,
  "rs485_baud": 9600,
  "espnow_enabled": false,
  "espnow_force_mode": "",
  "espnow_channel": 1,
  "beacon_interval_ms": 2000,
  "send_interval_ms": 30000,
  "grafana_ping_url": "http://192.168.1.1/ping",
  "sensors": [...]
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

**Response (éxito):**
```json
{"status": "success", "message": "SCD30 calibrado a 400ppm"}
```

**Response (error):**
```json
{"status": "error", "message": "Sensor no disponible o no es SCD30"}
```

**Códigos:**
- `200`: Calibración ejecutada
- `400`: Sensor no disponible o tipo incorrecto

**Recomendación:** Ejecutar con sensor en ambiente 400ppm CO2 (aire exterior)

---

### GET /settings

Formulario web de configuración.

**Response:** HTML con form completo
- Inputs para WiFi
- Selección de sensores
- Config RS485
- Config ESP-NOW
- Botones de gestión

**Funciones del form:**
- Submit POST a `/config`
- Botón "Restart" → POST a `/restart`
- Botón "Load Default Sensors" → carga config predefinida
- Muestra estado actual de ESP-NOW en tiempo real

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

**Response:**
```json
{
  "enabled": true,
  "mode": "gateway",
  "mac": "AA:BB:CC:DD:EE:FF",
  "channel": 6,
  "paired": true,
  "peer_count": 3,
  "gateway_mac": "11:22:33:44:55:66",
  "gateway_rssi": -45
}
```

**Campos:**
- `enabled`: ESP-NOW habilitado en config
- `mode`: "gateway", "sensor", o "disabled"
- `mac`: Dirección MAC del dispositivo
- `channel`: Canal WiFi actual
- `paired`: (solo sensor) Si encontró y se pareó con gateway
- `peer_count`: (solo gateway) Número de sensores pareados
- `gateway_mac`: (solo sensor) MAC del gateway pareado
- `gateway_rssi`: (solo sensor) Señal del gateway

**Códigos:**
- `200`: OK
- Si ESP-NOW disabled: `enabled: false`, resto null/0

---

### GET /favicon.svg

Ícono del sitio.

**Response:** SVG con logo AlterMundi
- Served from SPIFFS `/favicon.svg`
- Content-Type: image/svg+xml

**Códigos:**
- `200`: OK
- `404`: Archivo no encontrado en SPIFFS

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
