#include "webConfigPage.h"

const char* getConfigPageHTML() {
    return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Configuración - Monitor</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: Arial, sans-serif;
            background: #f5f5f5;
            padding: 20px;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            margin-bottom: 20px;
            border-bottom: 2px solid #4CAF50;
            padding-bottom: 10px;
        }
        h2 {
            color: #555;
            margin-top: 20px;
            margin-bottom: 10px;
            font-size: 18px;
        }
        .section {
            margin-bottom: 20px;
            padding: 15px;
            background: #fafafa;
            border-radius: 4px;
        }
        .form-group {
            margin-bottom: 15px;
        }
        label {
            display: block;
            margin-bottom: 5px;
            color: #555;
            font-weight: bold;
        }
        input[type="text"], input[type="number"], select {
            width: 100%;
            padding: 8px;
            border: 1px solid #ddd;
            border-radius: 4px;
            font-size: 14px;
        }
        input[type="checkbox"] {
            width: 18px;
            height: 18px;
            margin-right: 8px;
            vertical-align: middle;
        }
        .checkbox-label {
            display: inline;
            font-weight: normal;
        }
        .sensor-item {
            background: white;
            padding: 10px;
            margin-bottom: 10px;
            border-radius: 4px;
            border-left: 3px solid #4CAF50;
        }
        .btn {
            background: #4CAF50;
            color: white;
            padding: 12px 24px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 16px;
            margin-right: 10px;
        }
        .btn:hover { background: #45a049; }
        .btn-secondary {
            background: #666;
        }
        .btn-secondary:hover { background: #555; }
        .message {
            padding: 12px;
            margin-top: 15px;
            border-radius: 4px;
            display: none;
        }
        .message.success {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        .message.error {
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }
        .message.warning {
            background: #fff3cd;
            color: #856404;
            border: 1px solid #ffeeba;
        }
        .loading {
            text-align: center;
            padding: 20px;
            color: #666;
        }
        .info-text {
            color: #666;
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
                    </div>
                    <div class="form-group">
                        <label for="rs485_baud">Baudrate</label>
                        <select id="rs485_baud" name="rs485_baud">
                            <option value="9600">9600</option>
                            <option value="19200">19200</option>
                            <option value="38400">38400</option>
                            <option value="57600">57600</option>
                            <option value="115200">115200</option>
                        </select>
                    </div>
                </div>
            </div>

            <!-- Sensors Section -->
            <div class="section">
                <h2>Sensores</h2>
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
            document.getElementById('rs485_baud').value = config.rs485_baud || 9600;

            // Toggle RS485 config visibility
            toggleRS485Config();
            document.getElementById('rs485_enabled').addEventListener('change', toggleRS485Config);

            // Sensors
            renderSensors(config.sensors || []);
        }

        function toggleRS485Config() {
            const enabled = document.getElementById('rs485_enabled').checked;
            document.getElementById('rs485_config').style.display = enabled ? 'block' : 'none';
        }

        function renderSensors(sensors) {
            const container = document.getElementById('sensors-list');
            container.innerHTML = '';

            if (!Array.isArray(sensors) || sensors.length === 0) {
                container.innerHTML = '<div class="info-text">No hay sensores configurados</div>';
                return;
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
            config.rs485_baud = parseInt(document.getElementById('rs485_baud').value);

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
    </script>
</body>
</html>
)rawliteral";
}
