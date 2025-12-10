# Dashboard de Grafana - AlterMundi Monitoreo de Sensores

## 📊 Descripción

Dashboard interactivo de Grafana para visualizar datos de todos los sensores de temperatura, humedad y CO₂ del proyecto AlterMundi.

## 🎯 Características

### Variables Dinámicas
- **Dispositivo**: Filtra por dispositivo específico o ver todos
- **Tipo de Sensor**: Filtra por tipo de sensor (SCD30, BME280, DHT22, mesh_XXX, etc.)

### Paneles Incluidos

#### 1. **Resumen General** (Fila Superior)
- **Dispositivos Activos**: Cuenta total de dispositivos reportando datos
- **Temperatura Promedio**: Temperatura media con código de colores
- **Humedad Promedio**: Humedad media con rangos óptimos
- **CO₂ Promedio**: Nivel promedio de CO₂ con alertas

#### 2. **Gráficas Temporales**
- **Temperatura por Dispositivo**: Líneas de tiempo suavizadas por cada sensor
- **Humedad por Dispositivo**: Evolución de humedad en el tiempo
- **CO₂ por Dispositivo**: Monitoreo de calidad del aire con alertas automáticas

#### 3. **Tabla de Estado Actual**
Tabla con valores actuales de todos los sensores con:
- Colores de fondo según rangos de valores
- Última lectura de cada dispositivo
- Ordenable por cualquier columna

#### 4. **Mapa de Calor**
Distribución temporal de CO₂ para identificar patrones

## 🎨 Código de Colores

### Temperatura
- 🔵 **Azul** (< 18°C): Frío
- 🟢 **Verde** (18-25°C): Óptimo
- 🟡 **Amarillo** (25-30°C): Cálido
- 🔴 **Rojo** (> 30°C): Caliente

### Humedad
- 🔴 **Rojo** (< 30% o > 80%): Fuera de rango
- 🟡 **Amarillo** (30-40% o 70-80%): Límite
- 🟢 **Verde** (40-70%): Óptimo

### CO₂
- 🟢 **Verde** (< 800 ppm): Excelente
- 🟡 **Amarillo** (800-1000 ppm): Bueno
- 🟠 **Naranja** (1000-1500 ppm): Regular
- 🔴 **Rojo** (> 1500 ppm): Crítico - Ventilar

## 📥 Instalación

### Opción 1: Importación Manual (Recomendada)

1. **Acceder a Grafana**
   ```
   https://grafana.altermundi.net
   ```

2. **Importar Dashboard**
   - Ir a **Dashboards** → **Import** (botón `+` en la barra lateral)
   - Click en **Upload JSON file**
   - Seleccionar el archivo `grafana-dashboard.json`
   - Seleccionar la fuente de datos **InfluxDB** en el dropdown
   - Click en **Import**

### Opción 2: Importación vía API

```bash
curl -X POST \
  https://grafana.altermundi.net/api/dashboards/db \
  -H "Authorization: Bearer YOUR_API_KEY" \
  -H "Content-Type: application/json" \
  -d @grafana-dashboard.json
```

## 🔧 Configuración de Fuente de Datos

Asegúrate de tener configurada la fuente de datos de InfluxDB:

**Configuración necesaria:**
- **Tipo**: InfluxDB
- **URL**: `http://localhost:8086` (o la URL de tu instancia)
- **Database**: `cto`
- **HTTP Method**: GET

### Query de Prueba

Para verificar que los datos están llegando:

```sql
SELECT * FROM "medicionesCO2" WHERE time > now() - 1h
```

## 📊 Estructura de Datos

### Measurement: `medicionesCO2`

**Tags:**
- `device`: ID único del dispositivo (ej: `moni-80F3DAAD`)
- `sensor`: Tipo de sensor (ej: `SCD30`, `BME280`, `mesh_A1B2C3`)

**Fields:**
- `temp`: Temperatura en °C
- `hum`: Humedad en %
- `co2`: CO₂ en ppm

**Ejemplo de dato:**
```
medicionesCO2,device=moni-80F3DAAD,sensor=SCD30 temp=23.5,hum=60.2,co2=450 1699876543000000000
```

## ⚙️ Personalización

### Modificar Umbrales de Alerta CO₂

En el panel de CO₂, edita los thresholds:

```json
"thresholds": {
  "steps": [
    {"value": null, "color": "green"},
    {"value": 800, "color": "yellow"},    // Cambia este valor
    {"value": 1000, "color": "orange"},   // Cambia este valor
    {"value": 1500, "color": "red"}       // Cambia este valor
  ]
}
```

### Añadir Más Paneles

El dashboard usa **Grid Layout** de 24 columnas. Para añadir un panel:

1. Editar dashboard
2. Click en "Add panel"
3. Configurar query:
   ```sql
   SELECT mean("campo")
   FROM "medicionesCO2"
   WHERE ("device" =~ /^$device$/ AND "sensor" =~ /^$sensor$/)
     AND $timeFilter
   GROUP BY time($__interval), "device", "sensor"
   fill(linear)
   ```

### Variables Adicionales

Para añadir más filtros (ej: ubicación), edita `templating.list`:

```json
{
  "name": "ubicacion",
  "label": "Ubicación",
  "type": "query",
  "datasource": "InfluxDB",
  "query": "SHOW TAG VALUES FROM \"medicionesCO2\" WITH KEY = \"ubicacion\"",
  "multi": true,
  "includeAll": true
}
```

## 🚨 Alertas

### Alerta de CO₂ Alto

El panel de CO₂ incluye una alerta pre-configurada:
- **Condición**: CO₂ promedio > 1000 ppm durante 5 minutos
- **Frecuencia de evaluación**: 1 minuto
- **Estado sin datos**: No alertar

### Configurar Notificaciones

1. En Grafana: **Alerting** → **Notification channels**
2. Añadir canal (Email, Slack, Telegram, etc.)
3. En el panel de CO₂, editar alerta y asignar canal

## 📈 Uso del Dashboard

### Filtrar por Dispositivo
1. Click en dropdown "Dispositivo" en la parte superior
2. Selecciona uno o varios dispositivos
3. O selecciona "All" para ver todos

### Cambiar Rango de Tiempo
- Usa el selector de tiempo en la esquina superior derecha
- Rangos rápidos: 5m, 15m, 1h, 6h, 24h, 7d, 30d
- O selecciona rango personalizado

### Refrescar Datos
- Auto-refresh configurado a 30 segundos
- Cambia en el dropdown junto al selector de tiempo
- O click en el ícono de refresh manual

## 🔍 Troubleshooting

### No aparecen datos

1. **Verificar fuente de datos**:
   ```bash
   curl "http://grafana.altermundi.net:8086/query?db=cto&q=SELECT * FROM medicionesCO2 LIMIT 10"
   ```

2. **Verificar que los sensores están enviando datos**:
   - Revisar logs del ESP32
   - Verificar conectividad WiFi
   - Confirmar URL de Grafana en `constants_private.h`

3. **Query devuelve vacío**:
   - Ajustar rango de tiempo
   - Verificar filtros de variables
   - Revisar nombres de tags y fields

### Gráficas se ven cortadas

- Aumentar `fill(linear)` a `fill(previous)` o `fill(null)`
- Ajustar intervalo de agregación en `GROUP BY time($__interval)`

### Dashboard muy lento

- Reducir rango de tiempo
- Aumentar intervalo de refresh
- Limitar número de dispositivos mostrados

## 🌐 Acceso Remoto

Para acceder desde fuera de la red local:
1. Configurar port forwarding en router (puerto 3000 → Grafana)
2. O usar túnel SSH:
   ```bash
   ssh -L 3000:localhost:3000 user@grafana.altermundi.net
   ```
3. Acceder a `http://localhost:3000`

## 📝 Mantenimiento

### Backup del Dashboard

```bash
# Exportar dashboard actual
curl -H "Authorization: Bearer YOUR_API_KEY" \
  https://grafana.altermundi.net/api/dashboards/uid/DASHBOARD_UID \
  > backup-dashboard-$(date +%Y%m%d).json
```

### Actualizar Dashboard

1. Hacer cambios en el JSON local
2. Re-importar en Grafana
3. O usar API para actualizar

## 🤝 Contribuir

Para mejorar el dashboard:

1. Hacer cambios en `grafana-dashboard.json`
2. Probar en instancia local de Grafana
3. Documentar cambios en este README
4. Commit y PR al repositorio

## 📚 Referencias

- [Grafana Documentation](https://grafana.com/docs/)
- [InfluxDB Query Language](https://docs.influxdata.com/influxdb/v1.8/query_language/)
- [Panel Options Reference](https://grafana.com/docs/grafana/latest/panels/)

---

**Versión del Dashboard**: 1.0
**Compatible con**: Grafana 7.4+
**Base de Datos**: InfluxDB 1.x
**Proyecto**: AlterMundi - Monitoreo de Sensores
