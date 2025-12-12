#include "webConfigPage.h"

const char* getConfigPageHTML() {
    return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="icon" type="image/svg+xml" href="/favicon.svg">
    <title>Configuración - Monitor</title>
    <style>
        :root {
            --altermundi-green: #55d400;
            --altermundi-orange: #F39100;
            --altermundi-blue: #0198fe;
            --gray-dark: #333;
            --gray-medium: #666;
            --gray-light: #f5f5f5;
        }
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif;
            background: linear-gradient(135deg, #f5f5f5 0%, #e8e8e8 100%);
            padding: 20px;
            min-height: 100vh;
        }
        .container {
            max-width: 850px;
            margin: 0 auto;
            background: white;
            padding: 30px;
            border-radius: 12px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1), 0 1px 3px rgba(0,0,0,0.08);
        }
        h1 {
            color: var(--gray-dark);
            margin-bottom: 10px;
            border-bottom: 3px solid var(--altermundi-green);
            padding-bottom: 12px;
            font-size: 28px;
        }
        h1::before {
            content: "⚙️ ";
        }
        .subtitle {
            color: var(--gray-medium);
            font-size: 14px;
            margin-bottom: 25px;
        }
        h2 {
            color: var(--altermundi-green);
            margin-top: 20px;
            margin-bottom: 15px;
            font-size: 18px;
            font-weight: 600;
        }
        .section {
            margin-bottom: 25px;
            padding: 20px;
            background: #fafafa;
            border-radius: 8px;
            border-left: 4px solid var(--altermundi-green);
            transition: all 0.3s ease;
        }
        .section:hover {
            box-shadow: 0 2px 8px rgba(85, 212, 0, 0.15);
        }
        .form-group {
            margin-bottom: 18px;
        }
        label {
            display: block;
            margin-bottom: 6px;
            color: var(--gray-dark);
            font-weight: 600;
            font-size: 14px;
        }
        input[type="text"], input[type="number"], select {
            width: 100%;
            padding: 10px 12px;
            border: 2px solid #ddd;
            border-radius: 6px;
            font-size: 14px;
            transition: border-color 0.3s ease;
        }
        input[type="text"]:focus, input[type="number"]:focus, select:focus {
            outline: none;
            border-color: var(--altermundi-green);
        }
        input[type="checkbox"] {
            width: 20px;
            height: 20px;
            margin-right: 10px;
            vertical-align: middle;
            cursor: pointer;
        }
        .checkbox-label {
            display: inline;
            font-weight: normal;
            cursor: pointer;
        }
        .sensor-item {
            background: white;
            padding: 15px;
            margin-bottom: 12px;
            border-radius: 8px;
            border-left: 4px solid var(--altermundi-orange);
            box-shadow: 0 1px 3px rgba(0,0,0,0.08);
            transition: transform 0.2s ease;
        }
        .sensor-item:hover {
            transform: translateX(5px);
        }
        .btn {
            background: var(--altermundi-green);
            color: white;
            padding: 12px 28px;
            border: none;
            border-radius: 6px;
            cursor: pointer;
            font-size: 16px;
            font-weight: 600;
            margin-right: 10px;
            margin-top: 10px;
            transition: all 0.3s ease;
            box-shadow: 0 2px 4px rgba(85, 212, 0, 0.3);
        }
        .btn:hover {
            background: #48b800;
            transform: translateY(-2px);
            box-shadow: 0 4px 8px rgba(85, 212, 0, 0.4);
        }
        .btn-secondary {
            background: var(--gray-medium);
            box-shadow: 0 2px 4px rgba(0,0,0,0.2);
        }
        .btn-secondary:hover {
            background: #555;
            box-shadow: 0 4px 8px rgba(0,0,0,0.3);
        }
        .message {
            padding: 14px 18px;
            margin-top: 20px;
            border-radius: 8px;
            display: none;
            font-weight: 500;
        }
        .message.success {
            background: #d4f4dd;
            color: #1e7e34;
            border: 2px solid var(--altermundi-green);
        }
        .message.error {
            background: #ffe6e6;
            color: #c82333;
            border: 2px solid #dc3545;
        }
        .message.warning {
            background: #fff8e1;
            color: #856404;
            border: 2px solid var(--altermundi-orange);
        }
        .loading {
            text-align: center;
            padding: 40px 20px;
            color: var(--altermundi-green);
            font-size: 18px;
            font-weight: 500;
        }
        .info-text {
            color: var(--gray-medium);
            font-size: 12px;
            margin-top: 3px;
        }
        .inline-group {
            display: flex;
            gap: 10px;
        }
        .inline-group .form-group {
            flex: 1;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Configuración del Sistema</h1>
        <div class="subtitle">AlterMundi - La pata tecnológica de ese otro mundo posible</div>
        <div id="loading" class="loading">Cargando configuración...</div>

        <form id="configForm" style="display:none;">
            <!-- WiFi Section -->
            <div class="section">
                <h2>WiFi</h2>
                <div class="form-group">
                    <label for="ssid">SSID</label>
                    <input type="text" id="ssid" name="ssid" required>
                </div>
                <div class="form-group">
                    <label for="passwd">Contraseña</label>
                    <input type="text" id="passwd" name="passwd">
                    <div class="info-text">Dejar vacío para mantener contraseña actual</div>
                </div>
                <div class="form-group">
                    <label>Canal WiFi Actual, usar para esp now en todos los dispositivos espnow !!!!!! </label>
                    <div id="wifi_channel_status" style="padding: 10px; background: #f0f0f0; border-radius: 6px; color: var(--gray-dark); font-weight: 500;">-</div>
                    <div class="info-text">El canal en el que opera la red WiFi actual.</div>
                </div>
            </div>

            <!-- Sistema Section -->
            <div class="section">
                <h2>Sistema</h2>
                <div class="form-group">
                    <label for="incubator_name">Nombre del Dispositivo</label>
                    <input type="text" id="incubator_name" name="incubator_name">
                </div>
                <div class="inline-group">
                    <div class="form-group">
                        <label for="min_temperature">Temperatura Mín (°C)</label>
                        <input type="number" id="min_temperature" name="min_temperature" step="0.1">
                    </div>
                    <div class="form-group">
                        <label for="max_temperature">Temperatura Máx (°C)</label>
                        <input type="number" id="max_temperature" name="max_temperature" step="0.1">
                    </div>
                </div>
                <div class="inline-group">
                    <div class="form-group">
                        <label for="min_hum">Humedad Mín (%)</label>
                        <input type="number" id="min_hum" name="min_hum" min="0" max="100">
                    </div>
                    <div class="form-group">
                        <label for="max_hum">Humedad Máx (%)</label>
                        <input type="number" id="max_hum" name="max_hum" min="0" max="100">
                    </div>
                </div>
            </div>

            <!-- ESP-NOW Section -->
            <div class="section">
                <h2>ESP-NOW</h2>
                <div class="form-group">
                    <input type="checkbox" id="espnow_enabled" name="espnow_enabled">
                    <label class="checkbox-label" for="espnow_enabled">Habilitar ESP-NOW</label>
                </div>
                <div id="espnow_config">
                    <div class="form-group">
                        <label for="espnow_force_mode">Modo de Operación</label>
                        <select id="espnow_force_mode" name="espnow_force_mode">
                            <option value="">Auto-detectar (recomendado)</option>
                            <option value="gateway">Forzar Gateway</option>
                            <option value="sensor">Forzar Sensor</option>
                        </select>
                        <small style="color: var(--gray-medium);">Auto-detecta basándose en conectividad WiFi/Grafana</small>
                    </div>
                    <div class="form-group">
                        <label for="grafana_ping_url">URL de Prueba Grafana</label>
                        <input type="text" id="grafana_ping_url" name="grafana_ping_url" placeholder="http://192.168.1.1/ping">
                        <small style="color: var(--gray-medium);">URL para verificar conectividad a Grafana</small>
                    </div>
                    <div class="form-group">
                        <label for="espnow_channel">Canal WiFi !!!! </label>
                        <select id="espnow_channel" name="espnow_channel">
                            <option value="1">1</option>
                            <option value="6">6</option>
                            <option value="11">11</option>
                        </select>
                        <small style="color: var(--gray-medium);">Canal usado por gateways y sensores, igual al canal del WiFi del gateway</small>
                    </div>
                    <div class="form-group">
                        <label for="send_interval_ms">Intervalo de envío (ms)</label>
                        <input type="number" id="send_interval_ms" name="send_interval_ms" min="5000" max="300000" step="1000" value="30000">
                        <small style="color: var(--gray-medium);">Tiempo entre envíos de datos (solo sensores)</small>
                    </div>
                    <div id="espnow_status" style="margin-top: 15px; padding: 10px; background: #f9f9f9; border-radius: 4px;">
                        <strong>Estado:</strong> <span id="espnow_status_text">Cargando...</span><br>
                        <strong>Modo Actual:</strong> <span id="espnow_current_mode">-</span><br>
                        <strong>Canal WiFi Actual:</strong> <span id="espnow_channel_status">-</span><br>
                        <strong>MAC Address:</strong> <span id="espnow_mac">-</span><br>
                        <span id="espnow_paired_status"></span>
                        <span id="espnow_peer_count"></span>
                    </div>
                </div>
            </div>

            <!-- RS485 Section -->
            <div class="section">
                <h2>RS485</h2>
                <div class="form-group">
                    <input type="checkbox" id="rs485_enabled" name="rs485_enabled">
                    <label class="checkbox-label" for="rs485_enabled">Habilitar RS485</label>
                </div>
                <div id="rs485_config">
                    <div class="inline-group">
                        <div class="form-group">
                            <label for="rs485_rx">Pin RX</label>
                            <input type="number" id="rs485_rx" name="rs485_rx" min="0" max="39">
                        </div>
                        <div class="form-group">
                            <label for="rs485_tx">Pin TX</label>
                            <input type="number" id="rs485_tx" name="rs485_tx" min="0" max="39">
                        </div>
                        <div class="form-group">
                            <label for="rs485_de">Pin DE/RE</label>
                            <input type="number" id="rs485_de" name="rs485_de" min="-1" max="39">
                            <div class="info-text">-1 si no usa control DE/RE</div>
                        </div>
                    </div>
                    <div class="form-group">
                        <label for="rs485_baud">Baudrate</label>
                        <select id="rs485_baud" name="rs485_baud">
                            <option value="4800">4800</option>
                            <option value="9600">9600</option>
                            <option value="19200">19200</option>
                        </select>
                    </div>
                </div>
            </div>

            <!-- Sensors Section -->
            <div class="section">
                <h2>Sensores</h2>
                <div style="margin-bottom: 15px; display: flex; gap: 8px; flex-wrap: nowrap; align-items: center;">
                    <button type="button" class="btn" style="background: var(--altermundi-green); padding: 8px 12px; font-size: 13px;" onclick="loadDefaultSensors()">📋 Por Defecto</button>
                    <button type="button" class="btn btn-secondary" style="background: var(--altermundi-orange); padding: 8px 12px; font-size: 13px;" onclick="forgetWiFi()">📡 Olvidar WiFi</button>
                    <button type="button" class="btn btn-secondary" style="background: #dc3545; padding: 8px 12px; font-size: 13px;" onclick="clearAllConfig()">🗑️ Limpiar Todo</button>
                </div>
                <div id="sensors-list"></div>
            </div>

            <div class="message" id="message"></div>

            <button type="submit" class="btn">Guardar Configuración</button>
            <button type="button" class="btn btn-secondary" onclick="location.reload()">Recargar</button>
            <button type="button" class="btn btn-secondary" onclick="if(confirm('¿Reiniciar ESP32?')) restartDevice()">Reiniciar Dispositivo</button>
        </form>
    </div>

    <script>
        let currentConfig = {};

        // Default sensor configuration
        // Buses: I2C, Modbus, OneWire
        // Sensores: ADC, digital
        const DEFAULT_SENSORS = [
            // Buses
            { type: "scd30", enabled: true, config: {} },
            { type: "bme280", enabled: false, config: {} },
            { type: "modbus_th", enabled: false, config: { addresses: [1], rx_pin: 16, tx_pin: 17, de_pin: 18, baudrate: 9600 } },
            { type: "onewire", enabled: false, config: { pin: 4, scan: true } },
            // Sensores
            { type: "capacitive", enabled: false, config: { pin: 34, name: "Soil1" } },
            { type: "hd38", enabled: false, config: { analog_pin: 35, digital_pin: -1, voltage_divider: true, invert_logic: false, name: "Suelo1" } }
        ];

        // Load configuration on page load
        window.addEventListener('DOMContentLoaded', loadConfig);

        async function loadConfig() {
            try {
                const response = await fetch('/config');
                if (!response.ok) throw new Error('Error al cargar configuración');

                currentConfig = await response.json();
                populateForm(currentConfig);

                document.getElementById('loading').style.display = 'none';
                document.getElementById('configForm').style.display = 'block';
            } catch (error) {
                document.getElementById('loading').innerHTML =
                    '<div class="message error" style="display:block">Error: ' + error.message + '</div>';
            }
        }

        function populateForm(config) {
            // WiFi
            document.getElementById('ssid').value = config.ssid || '';
            document.getElementById('passwd').value = ''; // Never show password

            // WiFi Channel
            const channel = config.current_wifi_channel;
            document.getElementById('wifi_channel_status').textContent = channel > 0 ? channel : 'No conectado';

            // Sistema
            document.getElementById('incubator_name').value = config.incubator_name || config.moni_name || '';
            document.getElementById('min_temperature').value = config.min_temperature || '';
            document.getElementById('max_temperature').value = config.max_temperature || '';
            document.getElementById('min_hum').value = config.min_hum || '';
            document.getElementById('max_hum').value = config.max_hum || '';

            // RS485
            document.getElementById('rs485_enabled').checked = config.rs485_enabled || false;
            document.getElementById('rs485_rx').value = config.rs485_rx || 16;
            document.getElementById('rs485_tx').value = config.rs485_tx || 17;
            document.getElementById('rs485_de').value = config.rs485_de !== undefined ? config.rs485_de : 18;
            document.getElementById('rs485_baud').value = config.rs485_baud || 9600;

            // Toggle RS485 config visibility
            toggleRS485Config();
            document.getElementById('rs485_enabled').addEventListener('change', toggleRS485Config);

            // ESP-NOW
            document.getElementById('espnow_enabled').checked = config.espnow_enabled || false;
            document.getElementById('espnow_force_mode').value = config.espnow_force_mode || '';
            document.getElementById('grafana_ping_url').value = config.grafana_ping_url || 'http://192.168.1.1/ping';
            document.getElementById('espnow_channel').value = config.espnow_channel || 1;
            document.getElementById('send_interval_ms').value = config.send_interval_ms || 30000;

            // Toggle ESP-NOW config visibility
            toggleESPNowConfig();
            document.getElementById('espnow_enabled').addEventListener('change', toggleESPNowConfig);

            // Load ESP-NOW status
            loadESPNowStatus();

            // Sensors
            renderSensors(config.sensors || []);
        }

        function toggleRS485Config() {
            const enabled = document.getElementById('rs485_enabled').checked;
            document.getElementById('rs485_config').style.display = enabled ? 'block' : 'none';
        }

        function toggleESPNowConfig() {
            const enabled = document.getElementById('espnow_enabled').checked;
            document.getElementById('espnow_config').style.display = enabled ? 'block' : 'none';
        }

        async function loadESPNowStatus() {
            try {
                const response = await fetch('/espnow/status');
                if (!response.ok) {
                    document.getElementById('espnow_status_text').textContent = 'No disponible';
                    return;
                }

                const status = await response.json();
                document.getElementById('espnow_status_text').textContent = status.enabled ? 'Habilitado' : 'Deshabilitado';
                document.getElementById('espnow_current_mode').textContent = status.mode || '-';
                document.getElementById('espnow_mac').textContent = status.mac_address || '-';
                const channel = status.channel;
                document.getElementById('espnow_channel_status').textContent = channel > 0 ? channel : 'No disponible (sin WiFi)';

                if (status.mode === 'sensor') {
                    document.getElementById('espnow_paired_status').innerHTML =
                        '<strong>Emparejado:</strong> ' + (status.paired ? '✓ Sí' : '✗ No') + '<br>';
                    document.getElementById('espnow_peer_count').innerHTML = '';
                } else if (status.mode === 'gateway') {
                    document.getElementById('espnow_paired_status').innerHTML = '';
                    document.getElementById('espnow_peer_count').innerHTML =
                        '<strong>Peers conectados:</strong> ' + (status.peer_count || 0);
                } else {
                    document.getElementById('espnow_paired_status').innerHTML = '';
                    document.getElementById('espnow_peer_count').innerHTML = '';
                }
            } catch (error) {
                document.getElementById('espnow_status_text').textContent = 'Error al cargar estado';
            }
        }

        function renderSensors(sensors) {
            const container = document.getElementById('sensors-list');
            container.innerHTML = '';

            // Use default sensors if none configured
            if (!Array.isArray(sensors) || sensors.length === 0) {
                sensors = DEFAULT_SENSORS;
                container.innerHTML = '<div class="info-text" style="background: #fff3cd; border-left: 4px solid var(--altermundi-orange); padding: 12px; margin-bottom: 15px;">⚠️ Usando configuración por defecto. Guarda para aplicar.</div>';
            }

            sensors.forEach((sensor, index) => {
                const sensorDiv = document.createElement('div');
                sensorDiv.className = 'sensor-item';
                sensorDiv.innerHTML = `
                    <div class="form-group">
                        <input type="checkbox" id="sensor_${index}_enabled"
                               data-sensor-index="${index}" class="sensor-enabled"
                               ${sensor.enabled ? 'checked' : ''}>
                        <label class="checkbox-label" for="sensor_${index}_enabled">
                            <strong>${sensor.type.toUpperCase()}</strong>
                        </label>
                    </div>
                    ${renderSensorConfig(sensor, index)}
                `;
                container.appendChild(sensorDiv);
            });
        }

        function renderSensorConfig(sensor, index) {
            const config = sensor.config || {};

            switch(sensor.type) {
                case 'capacitive':
                    return `
                        <div class="form-group">
                            <label for="sensor_${index}_pin">Pin ADC</label>
                            <input type="number" id="sensor_${index}_pin"
                                   value="${config.pin || 34}" min="0" max="39">
                        </div>
                    `;

                case 'onewire':
                    return `
                        <div class="form-group">
                            <label for="sensor_${index}_pin">Pin Data</label>
                            <input type="number" id="sensor_${index}_pin"
                                   value="${config.pin || 4}" min="0" max="39">
                        </div>
                        <div class="form-group">
                            <input type="checkbox" id="sensor_${index}_scan"
                                   ${config.scan ? 'checked' : ''}>
                            <label class="checkbox-label" for="sensor_${index}_scan">
                                Auto-detectar sensores DS18B20
                            </label>
                        </div>
                    `;

                case 'modbus_th':
                    // Support both 'addresses' array and legacy 'address' single value
                    const addrList = config.addresses || (config.address ? [config.address] : [1]);
                    const addrStr = Array.isArray(addrList) ? addrList.join(', ') : addrList;
                    return `
                        <div class="form-group">
                            <label for="sensor_${index}_addresses">Direcciones Modbus</label>
                            <input type="text" id="sensor_${index}_addresses"
                                   value="${addrStr}" placeholder="1, 45, 3">
                            <div class="info-text">Separar con comas para multiples sensores en el mismo bus</div>
                        </div>
                        <div class="inline-group">
                            <div class="form-group">
                                <label for="sensor_${index}_baudrate">Baudrate</label>
                                <select id="sensor_${index}_baudrate">
                                    <option value="4800" ${config.baudrate == 4800 ? 'selected' : ''}>4800</option>
                                    <option value="9600" ${config.baudrate == 9600 ? 'selected' : ''}>9600</option>
                                    <option value="19200" ${config.baudrate == 19200 ? 'selected' : ''}>19200</option>
                                </select>
                            </div>
                            <div class="form-group">
                                <label for="sensor_${index}_de_pin">Pin DE/RE</label>
                                <input type="number" id="sensor_${index}_de_pin"
                                       value="${config.de_pin !== undefined ? config.de_pin : 18}" min="-1" max="39">
                                <div class="info-text">-1 si no usa</div>
                            </div>
                        </div>
                        <div class="inline-group">
                            <div class="form-group">
                                <label for="sensor_${index}_rx_pin">Pin RX</label>
                                <input type="number" id="sensor_${index}_rx_pin"
                                       value="${config.rx_pin || 16}" min="0" max="39">
                            </div>
                            <div class="form-group">
                                <label for="sensor_${index}_tx_pin">Pin TX</label>
                                <input type="number" id="sensor_${index}_tx_pin"
                                       value="${config.tx_pin || 17}" min="0" max="39">
                            </div>
                        </div>
                    `;

                case 'hd38':
                    return `
                        <div class="form-group">
                            <label for="sensor_${index}_name">Nombre</label>
                            <input type="text" id="sensor_${index}_name"
                                   value="${config.name || 'Suelo1'}" placeholder="Suelo1">
                        </div>
                        <div class="inline-group">
                            <div class="form-group">
                                <label for="sensor_${index}_analog_pin">Pin Analógico</label>
                                <input type="number" id="sensor_${index}_analog_pin"
                                       value="${config.analog_pin || 35}" min="0" max="39">
                            </div>
                            <div class="form-group">
                                <label for="sensor_${index}_digital_pin">Pin Digital</label>
                                <input type="number" id="sensor_${index}_digital_pin"
                                       value="${config.digital_pin !== undefined ? config.digital_pin : -1}" min="-1" max="39">
                                <div class="info-text">-1 para desactivar</div>
                            </div>
                        </div>
                        <div class="form-group">
                            <input type="checkbox" id="sensor_${index}_voltage_divider"
                                   ${config.voltage_divider !== false ? 'checked' : ''}>
                            <label class="checkbox-label" for="sensor_${index}_voltage_divider">
                                Usar divisor de voltaje (sensor 5V)
                            </label>
                        </div>
                        <div class="form-group">
                            <input type="checkbox" id="sensor_${index}_invert_logic"
                                   ${config.invert_logic ? 'checked' : ''}>
                            <label class="checkbox-label" for="sensor_${index}_invert_logic">
                                Invertir lógica digital
                            </label>
                        </div>
                    `;

                default:
                    return '<div class="info-text">Sin configuración adicional</div>';
            }
        }

        // Form submission
        document.getElementById('configForm').addEventListener('submit', async (e) => {
            e.preventDefault();

            const formData = buildConfigFromForm();

            try {
                const response = await fetch('/config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(formData)
                });

                const result = await response.text();

                if (response.ok) {
                    showMessage('Configuración guardada correctamente. Algunos cambios requieren reiniciar el dispositivo.', 'success');
                } else {
                    showMessage('Error al guardar: ' + result, 'error');
                }
            } catch (error) {
                showMessage('Error de conexión: ' + error.message, 'error');
            }
        });

        function buildConfigFromForm() {
            const config = { ...currentConfig };

            // WiFi
            config.ssid = document.getElementById('ssid').value;
            const passwd = document.getElementById('passwd').value;
            if (passwd) config.passwd = passwd;

            // Sistema
            config.incubator_name = document.getElementById('incubator_name').value;
            config.min_temperature = parseFloat(document.getElementById('min_temperature').value);
            config.max_temperature = parseFloat(document.getElementById('max_temperature').value);
            config.min_hum = parseInt(document.getElementById('min_hum').value);
            config.max_hum = parseInt(document.getElementById('max_hum').value);

            // RS485
            config.rs485_enabled = document.getElementById('rs485_enabled').checked;
            config.rs485_rx = parseInt(document.getElementById('rs485_rx').value);
            config.rs485_tx = parseInt(document.getElementById('rs485_tx').value);
            config.rs485_de = parseInt(document.getElementById('rs485_de').value);
            config.rs485_baud = parseInt(document.getElementById('rs485_baud').value);

            // ESP-NOW
            config.espnow_enabled = document.getElementById('espnow_enabled').checked;
            config.espnow_force_mode = document.getElementById('espnow_force_mode').value;
            config.grafana_ping_url = document.getElementById('grafana_ping_url').value;
            config.espnow_channel = parseInt(document.getElementById('espnow_channel').value);
            config.send_interval_ms = parseInt(document.getElementById('send_interval_ms').value);

            // Sensors
            if (Array.isArray(config.sensors)) {
                config.sensors.forEach((sensor, index) => {
                    const enabledCheckbox = document.getElementById(`sensor_${index}_enabled`);
                    if (enabledCheckbox) {
                        sensor.enabled = enabledCheckbox.checked;
                    }

                    const pinInput = document.getElementById(`sensor_${index}_pin`);
                    if (pinInput && sensor.config) {
                        sensor.config.pin = parseInt(pinInput.value);
                    }

                    if (sensor.type === 'onewire') {
                        const scanCheckbox = document.getElementById(`sensor_${index}_scan`);
                        if (scanCheckbox && sensor.config) {
                            sensor.config.scan = scanCheckbox.checked;
                        }
                    }

                    if (sensor.type === 'modbus_th') {
                        if (!sensor.config) sensor.config = {};
                        const addrInput = document.getElementById(`sensor_${index}_addresses`);
                        const baudInput = document.getElementById(`sensor_${index}_baudrate`);
                        const rxInput = document.getElementById(`sensor_${index}_rx_pin`);
                        const txInput = document.getElementById(`sensor_${index}_tx_pin`);
                        const deInput = document.getElementById(`sensor_${index}_de_pin`);

                        // Parse comma-separated addresses into array
                        if (addrInput) {
                            const addrStr = addrInput.value.trim();
                            const addrArray = addrStr.split(',')
                                .map(s => parseInt(s.trim()))
                                .filter(n => !isNaN(n) && n >= 1 && n <= 254);
                            sensor.config.addresses = addrArray.length > 0 ? addrArray : [1];
                            delete sensor.config.address;  // Remove legacy field
                        }
                        if (baudInput) sensor.config.baudrate = parseInt(baudInput.value);
                        if (rxInput) sensor.config.rx_pin = parseInt(rxInput.value);
                        if (txInput) sensor.config.tx_pin = parseInt(txInput.value);
                        if (deInput) sensor.config.de_pin = parseInt(deInput.value);
                    }

                    if (sensor.type === 'hd38') {
                        if (!sensor.config) sensor.config = {};
                        const nameInput = document.getElementById(`sensor_${index}_name`);
                        const analogPinInput = document.getElementById(`sensor_${index}_analog_pin`);
                        const digitalPinInput = document.getElementById(`sensor_${index}_digital_pin`);
                        const voltageDividerInput = document.getElementById(`sensor_${index}_voltage_divider`);
                        const invertLogicInput = document.getElementById(`sensor_${index}_invert_logic`);

                        if (nameInput) sensor.config.name = nameInput.value;
                        if (analogPinInput) sensor.config.analog_pin = parseInt(analogPinInput.value);
                        if (digitalPinInput) sensor.config.digital_pin = parseInt(digitalPinInput.value);
                        if (voltageDividerInput) sensor.config.voltage_divider = voltageDividerInput.checked;
                        if (invertLogicInput) sensor.config.invert_logic = invertLogicInput.checked;
                    }
                });
            }

            return config;
        }

        function showMessage(text, type) {
            const msg = document.getElementById('message');
            msg.textContent = text;
            msg.className = 'message ' + type;
            msg.style.display = 'block';

            setTimeout(() => {
                msg.style.display = 'none';
            }, 5000);
        }

        async function restartDevice() {
            try {
                await fetch('/restart', { method: 'POST' });
                showMessage('Reiniciando dispositivo...', 'warning');
                setTimeout(() => location.reload(), 5000);
            } catch (error) {
                showMessage('Dispositivo reiniciando', 'warning');
            }
        }

        async function loadDefaultSensors() {
            if (!confirm('¿Cargar configuración de sensores por defecto? (SCD30, BME280, Capacitive, OneWire)')) {
                return;
            }

            currentConfig.sensors = DEFAULT_SENSORS;
            renderSensors(currentConfig.sensors);
            showMessage('Sensores por defecto cargados. Guarda la configuración para aplicar.', 'success');
        }

        async function forgetWiFi() {
            if (!confirm('¿Olvidar red WiFi actual? El dispositivo volverá a modo AP.')) {
                return;
            }

            try {
                currentConfig.ssid = '';
                currentConfig.passwd = '';

                const response = await fetch('/config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(currentConfig)
                });

                if (!response.ok) throw new Error('Error al guardar');

                showMessage('WiFi olvidado. Reiniciando en modo AP...', 'success');
                setTimeout(() => location.href = '/', 3000);
            } catch (error) {
                showMessage('Error al olvidar WiFi: ' + error.message, 'error');
            }
        }

        async function clearAllConfig() {
            if (!confirm('⚠️ ¿Limpiar TODA la configuración? Esto borrará todos los ajustes y reiniciará el dispositivo.')) {
                return;
            }

            if (!confirm('¿Estás SEGURO? Esta acción no se puede deshacer.')) {
                return;
            }

            try {
                const response = await fetch('/config/reset', { method: 'POST' });

                if (!response.ok) throw new Error('Error al resetear');

                showMessage('Configuración limpiada. Reiniciando dispositivo...', 'success');
                setTimeout(() => location.href = '/', 5000);
            } catch (error) {
                showMessage('Configuración reseteada. Dispositivo reiniciando...', 'warning');
                setTimeout(() => location.href = '/', 5000);
            }
        }
    </script>
</body>
</html>
)rawliteral";
}
