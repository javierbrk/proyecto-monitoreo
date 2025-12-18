#ifndef ESPNOW_MANAGER_H
#define ESPNOW_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// Message types for ESP-NOW communication
enum MessageType {
  MSG_BEACON = 0,        // Gateway beacon broadcast
  MSG_PAIR_REQUEST = 1,  // Sensor pairing request
  MSG_PAIR_ACK = 2,      // Gateway pairing acknowledgement
  MSG_DATA = 3          // Sensor data transmission
};

// Discovery/pairing message structure
typedef struct {
  uint8_t msgType;       // MessageType enum
  uint8_t deviceId;      // Device identifier (derived from MAC)
  uint8_t macAddr[6];    // Sender's MAC address
  uint8_t channel;       // WiFi channel
  int8_t rssi;           // Signal strength (for gateway selection)
  uint32_t timestamp;    // Timestamp for timeout detection
} DiscoveryMessage;

// Sensor data message structure
typedef struct {
  uint8_t msgType;       // MessageType enum (MSG_DATA)
  uint8_t hopCount;      // Hop limit for flooding
  uint8_t originatorMAC[6]; // Original sender's MAC
  char sensorId[32];      // Sensor identifier
  float temperature;     // Temperature reading(-1 if not available)
  float humidity;        // Humidity reading (-1 if not available)
  float co2;            // CO2 reading (-1 if not available)
  uint32_t sequence;     // Sequence number for duplicate detection

} SensorDataMessage;

// Peer information structure
struct PeerInfo {
  uint8_t mac[6];
  uint32_t lastSeen;
  bool active;
};

// Pairing states
enum PairingState {
  NOT_PAIRED = 0,
  PAIRING = 1,
  PAIRED = 2
};

class ESPNowManager {
private:
  // Configuration
  String mode;                    // "gateway" or "sensor"
  bool enabled;
  uint8_t channel;
  uint16_t beaconInterval;
  uint16_t discoveryTimeout;
  uint16_t sendInterval;

  // Pairing state
  PairingState pairingState;
  uint8_t gatewayMAC[6];
  int8_t bestGatewayRSSI;
  uint32_t lastBeaconTime;
  uint32_t lastDiscoveryAttempt;
  uint32_t sequenceNumber;

  // Peer management (for gateway)
  static const int MAX_PEERS = 20;
  PeerInfo peers[MAX_PEERS];
  int peerCount;
  uint32_t lastPeerCleanup;

  // Broadcast address
  uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  // Static instance pointer for callbacks
  static ESPNowManager* instance;

  // Duplicate packet detection cache for flooding
  static const int SEEN_PACKET_CACHE_SIZE = 30;
  struct PacketID {
    uint8_t mac[6];
    uint32_t sequence;
    unsigned long timestamp;
  };
  PacketID seenPackets[SEEN_PACKET_CACHE_SIZE];
  int seenPacketIndex;
  // Callback for received mesh data (gateway only)
  typedef void (*MeshDataCallback)(const uint8_t* senderMAC, float temp, float hum, float co2, uint32_t seq, const char* sensorId);
  MeshDataCallback meshDataCallback;

  // Cleanup stale peers (gateway only)
  void cleanupStalePeers() {
    if (mode != "gateway") return;

    uint32_t now = millis();
    const uint32_t PEER_TIMEOUT = 300000;  // 5 minutes

    for (int i = 0; i < peerCount; i++) {
      if (peers[i].active && (now - peers[i].lastSeen) > PEER_TIMEOUT) {
        Serial.printf("[ESP-NOW] Peer %d timeout, removido\n", i);
        esp_now_del_peer(peers[i].mac);
        peers[i].active = false;
      }
    }
  }

  // Find peer index by MAC address
  int findPeerIndex(const uint8_t* mac) {
    for (int i = 0; i < MAX_PEERS; i++) {
      if (peers[i].active && memcmp(peers[i].mac, mac, 6) == 0) {
        return i;
      }
    }
    return -1;
  }

  // Add peer to list
  bool addPeerToList(const uint8_t* mac) {
    // Check if peer already exists
    int existingIndex = findPeerIndex(mac);
    if (existingIndex >= 0) {
      peers[existingIndex].lastSeen = millis();
      return true;
    }

    // Find empty slot
    for (int i = 0; i < MAX_PEERS; i++) {
      if (!peers[i].active) {
        memcpy(peers[i].mac, mac, 6);
        peers[i].lastSeen = millis();
        peers[i].active = true;
        if (i >= peerCount) peerCount = i + 1;
        return true;
      }
    }

    Serial.println("[ESP-NOW] Límite de peers alcanzado");
    return false;
  }

  // Check if a packet has been seen recently to prevent loops
  bool hasSeenPacket(const uint8_t* mac, uint32_t seq) {
    unsigned long now = millis();
    const unsigned long SEEN_TIMEOUT = 60000; // 1 minute timeout

    for (int i = 0; i < SEEN_PACKET_CACHE_SIZE; i++) {
        if (memcmp(seenPackets[i].mac, mac, 6) == 0 && seenPackets[i].sequence == seq) {
            if (now - seenPackets[i].timestamp < SEEN_TIMEOUT) {
                return true; // Packet is fresh and a duplicate
            }
        }
    }
    return false; // Packet not seen or is stale
  }

  // Mark a packet as seen
  void markPacketAsSeen(const uint8_t* mac, uint32_t seq) {
      memcpy(seenPackets[seenPacketIndex].mac, mac, 6);
      seenPackets[seenPacketIndex].sequence = seq;
      seenPackets[seenPacketIndex].timestamp = millis();
      seenPacketIndex = (seenPacketIndex + 1) % SEEN_PACKET_CACHE_SIZE;
  }

  // Static callback handlers
  static void onDataSentStatic(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (instance) {
      instance->onDataSent(mac_addr, status);
    }
  }

  static void onDataRecvStatic(const uint8_t *mac_addr, const uint8_t *data, int len) {
    if (instance) {
      instance->onDataRecv(mac_addr, data, len);
    }
  }

  // Instance callback handlers
  void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    // Keep minimal - runs on WiFi task
    if (status != ESP_NOW_SEND_SUCCESS) {
      Serial.println("[ESP-NOW] ✗ Envío fallido");
    }
  }

  void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int len) {
    // Keep minimal - runs on WiFi task
    if (len < sizeof(uint8_t)) return;

    uint8_t msgType = data[0];

    if (mode == "sensor" && msgType == MSG_BEACON && pairingState != PAIRED) {
      // Sensor received beacon from gateway
      handleBeaconReceived(mac_addr, data, len);

    } else if (mode == "sensor" && msgType == MSG_PAIR_ACK && pairingState == PAIRING) {
      // Sensor received pairing acknowledgement
      handlePairAckReceived(mac_addr, data, len);

    } else if (msgType == MSG_PAIR_REQUEST) {
      // Gateway or Sensor received a pairing request
      handlePairRequestReceived(mac_addr, data, len);

    } else if (msgType == MSG_DATA) {
      // Any node can receive sensor data for forwarding
      handleDataReceived(mac_addr, data, len);
    }
  }

  void handleBeaconReceived(const uint8_t *mac_addr, const uint8_t *data, int len) {
    if (len != sizeof(DiscoveryMessage)) return;

    DiscoveryMessage* msg = (DiscoveryMessage*)data;
    int8_t rssi = msg->rssi;

    Serial.printf("[ESP-NOW] Beacon %02X:%02X:%02X:%02X:%02X:%02X (RSSI: %d dBm)\n",
                  mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5], rssi);

    // Multi-gateway support: choose gateway with best RSSI
    if (pairingState != PAIRED || rssi > bestGatewayRSSI + 10) {  // 10 dBm hysteresis
      if (rssi > bestGatewayRSSI || pairingState != PAIRED) {
        Serial.printf("  └─ Mejor peer encontrado (RSSI: %d vs %d)\n", rssi, bestGatewayRSSI);

        // Save peer info
        memcpy(gatewayMAC, mac_addr, 6);
        bestGatewayRSSI = rssi;

        // Send pairing request
        DiscoveryMessage pairReq;
        pairReq.msgType = MSG_PAIR_REQUEST;
        pairReq.deviceId = ESP.getEfuseMac() & 0xFF;
        WiFi.macAddress(pairReq.macAddr);
        pairReq.channel = WiFi.channel();
        pairReq.rssi = 0;  // Not used in request
        pairReq.timestamp = millis();

        // Add random delay to avoid collisions
        delayMicroseconds(random(0, 500));

        // IMPORTANT: Send as broadcast since gateway is not in peer list yet
        // Gateway will receive it and extract sender MAC from data payload
        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&pairReq, sizeof(pairReq));
        pairingState = PAIRING;

        if (result == ESP_OK) {
          Serial.println("  └─ Pairing request enviado (broadcast)");
        } else {
          Serial.printf("  └─ ✗ Pairing request falló: %d\n", result);
        }
      }
    }
  }

  void handlePairAckReceived(const uint8_t *mac_addr, const uint8_t *data, int len) {
    if (len != sizeof(DiscoveryMessage)) return;

    DiscoveryMessage* msg = (DiscoveryMessage*)data;

    Serial.println("[ESP-NOW] ✓ Pairing ACK recibido");

    // Add the peer that sent the ACK
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, gatewayMAC, 6);
    peerInfo.channel = msg->channel;
    peerInfo.encrypt = false;

    if (!esp_now_is_peer_exist(gatewayMAC)) {
      esp_err_t result = esp_now_add_peer(&peerInfo);
      if (result == ESP_OK) {
        pairingState = PAIRED;
        Serial.println("  └─ ✓ Emparejado con peer exitosamente");
      } else {
        Serial.printf("  └─ ✗ Error agregando peer: %d\n", result);
      }
    } else {
      pairingState = PAIRED;
      Serial.println("  └─ ✓ Peer ya en lista, emparejado");
    }
  }

  void handlePairRequestReceived(const uint8_t *mac_addr, const uint8_t *data, int len) {
    if (len != sizeof(DiscoveryMessage)) return;

    Serial.printf("[ESP-NOW] Pairing request: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

    // Add to peer list
    if (!addPeerToList(mac_addr)) {
      Serial.println("  └─ ✗ Límite de peers alcanzado");
      return;
    }

    // Add as ESP-NOW peer if not already added
    if (!esp_now_is_peer_exist(mac_addr)) {
      esp_now_peer_info_t peerInfo = {};
      memcpy(peerInfo.peer_addr, mac_addr, 6);
      peerInfo.channel = channel; // Use configured channel
      peerInfo.encrypt = false;

      esp_err_t result = esp_now_add_peer(&peerInfo);
      if (result == ESP_OK) {
        Serial.printf("  └─ ✓ Peer agregado (total: %d)\n", getActivePeerCount());
      } else {
        Serial.printf("  └─ ✗ Error agregando peer: %d\n", result);
        return;
      }
    }

    // Send pairing acknowledgement
    DiscoveryMessage ack;
    ack.msgType = MSG_PAIR_ACK;
    if (mode == "gateway") {
      WiFi.softAPmacAddress(ack.macAddr);  // Gateway uses its SoftAP MAC
    } else {
      WiFi.macAddress(ack.macAddr);        // Sensor uses its Station MAC
    }
    ack.channel = channel; // Use configured channel

    esp_now_send(mac_addr, (uint8_t*)&ack, sizeof(ack));
    Serial.println("  └─ ✓ ACK enviado");
  }

  void handleDataReceived(const uint8_t *mac_addr, const uint8_t *data, int len) {
    if (len != sizeof(SensorDataMessage)) return;

    // Create a mutable copy of the message for forwarding
    SensorDataMessage msg;
    memcpy(&msg, data, sizeof(SensorDataMessage));

    // 1. Duplicate check to prevent loops and storms
    if (hasSeenPacket(msg.originatorMAC, msg.sequence)) {
      return; // Drop duplicate packet
    }
    markPacketAsSeen(msg.originatorMAC, msg.sequence);

    // 2. If this node is a gateway, process the data
    if (mode == "gateway" && meshDataCallback != nullptr) {
      Serial.printf("[ESP-NOW] Gateway got data from %s. Hops left: %d\n", msg.sensorId, msg.hopCount);
      // Pass originator's MAC to the callback
      meshDataCallback(msg.originatorMAC, msg.temperature, msg.humidity, msg.co2, msg.sequence, msg.sensorId);
    }

    // 3. Forward the packet if hop limit is not reached
    if (msg.hopCount > 1) {
      msg.hopCount--; // Decrement hop count

      // Re-broadcast the modified message to all neighbors
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&msg, sizeof(msg));
      if (result != ESP_OK) {
        // Optional: log forwarding failure
      }
    }
  }

public:
  ESPNowManager()
    : mode("sensor"), enabled(false), channel(1), beaconInterval(2000),
      discoveryTimeout(15000), sendInterval(30000), pairingState(NOT_PAIRED),
      bestGatewayRSSI(-100), lastBeaconTime(0), lastDiscoveryAttempt(0),
      sequenceNumber(0), peerCount(0), lastPeerCleanup(0), seenPacketIndex(0),
      meshDataCallback(nullptr) {
    memset(gatewayMAC, 0, 6);
    memset(peers, 0, sizeof(peers));
    memset(seenPackets, 0, sizeof(seenPackets));
    instance = this;
  }

  // Initialize ESP-NOW
  bool init(const String& operationMode = "sensor", uint8_t wifiChannel = 1) {
    mode = operationMode;

    // Validate channel is in valid range (1-13)
    if (wifiChannel < 1 || wifiChannel > 13) {
      Serial.printf("[ESP-NOW] ✗ Canal inválido %d (debe ser 1-13)\n", wifiChannel);
      return false;
    }
    channel = wifiChannel;

    Serial.printf("[ESP-NOW] Inicializando modo %s en canal %d\n", mode.c_str(), channel);

    // WiFi must be initialized before ESP-NOW
    if (mode == "gateway") {
      // Gateway uses AP_STA mode (already set up by WiFiManager)
      Serial.println("  └─ Gateway: usando config WiFi existente");
    } else {
      // Sensor mode: WiFi STA without connection
      //WiFi.mode(WIFI_STA);
      //WiFi.disconnect();

      // CRITICAL: Force WiFi channel for sensor mode
      // Without this, sensor won't receive gateway beacons on specific channel
      esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
      Serial.printf("  └─ Sensor: WiFi STA forzado a canal %d\n", channel);
    }

    // Initialize ESP-NOW
    esp_err_t result = esp_now_init();
    if (result != ESP_OK) {
      Serial.printf("[ESP-NOW] ✗ Init falló: %d\n", result);
      return false;
    }

    // Register callbacks
    esp_now_register_send_cb(onDataSentStatic);
    esp_now_register_recv_cb(onDataRecvStatic);

    // For broadcast (needed for beacon and initial pairing)
    esp_now_peer_info_t broadcastPeer = {};
    memcpy(broadcastPeer.peer_addr, broadcastAddress, 6);
    broadcastPeer.channel = 0;  // Use current channel
    broadcastPeer.encrypt = false;
    esp_now_add_peer(&broadcastPeer);

    enabled = true;
    Serial.println("[ESP-NOW] ✓ Inicializado exitosamente");

    if (mode == "gateway") {
      Serial.printf("  └─ MAC Gateway (SoftAP): %s\n", WiFi.softAPmacAddress().c_str());
    } else {
      Serial.printf("  └─ MAC Sensor (Station): %s\n", WiFi.macAddress().c_str());
    }

    return true;
  }

  // Register callback for received mesh data (gateway only)
  void setMeshDataCallback(MeshDataCallback callback) {
    meshDataCallback = callback;
  }

  // Broadcast beacon (gateway and sensors)
  void broadcastBeacon() {
    if (!enabled) return;

    uint32_t now = millis();
    if (now - lastBeaconTime < beaconInterval) return;

    DiscoveryMessage beacon;
    beacon.msgType = MSG_BEACON;
    beacon.deviceId = ESP.getEfuseMac() & 0xFF;

    // Use SoftAP MAC for gateway, Station MAC for sensor
    if (mode == "gateway") {
      String macStr = WiFi.softAPmacAddress();
      if (macStr.length() > 0) {
        sscanf(macStr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &beacon.macAddr[0], &beacon.macAddr[1], &beacon.macAddr[2], &beacon.macAddr[3], &beacon.macAddr[4], &beacon.macAddr[5]);
      } else {
        WiFi.macAddress(beacon.macAddr); // Fallback to station MAC
      }
    } else {
      WiFi.macAddress(beacon.macAddr);
    }

    beacon.channel = channel;  // Use configured channel instead of WiFi.channel()

    if (WiFi.status() == WL_CONNECTED) {
      beacon.rssi = WiFi.RSSI();
    } else {
      beacon.rssi = -50;  // Default moderate signal strength
    }
    beacon.timestamp = now;

    esp_now_send(broadcastAddress, (uint8_t*)&beacon, sizeof(beacon));
    lastBeaconTime = now;

    // Periodic peer cleanup (gateway only)
    if (mode == "gateway" && (now - lastPeerCleanup > 60000)) {  // Every minute
      cleanupStalePeers();
      lastPeerCleanup = now;
    }
  }

  // Wait for gateway discovery (sensor only)
  bool waitForDiscovery() {
    if (!enabled || mode != "sensor") return false;

    Serial.println("[ESP-NOW] Listening for gateway beacon...");

    unsigned long startTime = millis();
    while (pairingState != PAIRED && millis() - startTime < discoveryTimeout) {
      delay(100);
    }

    if (pairingState == PAIRED) {
      Serial.println("[ESP-NOW] Discovery successful!");
      return true;
    } else {
      Serial.println("[ESP-NOW] Discovery timeout");
      return false;
    }
  }

  // Retry discovery if connection lost (sensor only)
  void retryDiscoveryIfNeeded() {
    if (!enabled || mode != "sensor") return;
    if (pairingState == PAIRED) return;

    uint32_t now = millis();
    if (now - lastDiscoveryAttempt > 30000) {  // Retry every 30 seconds
      Serial.println("[ESP-NOW] Retrying discovery...");
      lastDiscoveryAttempt = now;
      waitForDiscovery();
    }
  }

  // Send sensor data (sensor only)
  bool sendSensorData(float temperature, float humidity, float co2, const char* sensorId) {
    // In a flooding mesh, we don't need to be "paired" to send. We just broadcast.
    if (!enabled || mode != "sensor") {
      return false;
    }

    SensorDataMessage msg;
    msg.msgType = MSG_DATA;
    msg.hopCount = 4; // Set initial hop limit (e.g., 4)
    WiFi.macAddress(msg.originatorMAC); // This node is the originator
    strncpy(msg.sensorId, sensorId, sizeof(msg.sensorId) - 1);
    msg.sensorId[sizeof(msg.sensorId) - 1] = '\0'; // Asegurar null-termination    msg.temperature = temperature;
    msg.humidity = humidity;
    msg.co2 = co2;
    msg.sequence = sequenceNumber++;

    // Broadcast the data to all listening peers
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&msg, sizeof(msg));

    if (result == ESP_OK) {
      Serial.printf("[ESP-NOW] Data broadcasted: T=%.1f H=%.1f CO2=%.0f\n",
                    temperature, humidity, co2);
      return true;
    } else {
      Serial.printf("[ESP-NOW] Broadcast failed: %d\n", result);
      return false;
    }
  }

  // Get pairing status
  bool isPaired() const {
    return pairingState == PAIRED;
  }

  // Get mode
  String getMode() const {
    return mode;
  }

  // Get peer count (gateway only)
  int getPeerCount() const {
    return peerCount;
  }

  // Get active peer count (gateway only)
  int getActivePeerCount() const {
    int count = 0;
    for (int i = 0; i < MAX_PEERS; i++) {
      if (peers[i].active) count++;
    }
    return count;
  }

  // Get MAC address as string
  String getMACAddress() const {
    if (mode == "gateway") {
      return WiFi.softAPmacAddress();
    } else {
      return WiFi.macAddress();
    }
  }

  // Check if enabled
  bool isEnabled() const {
    return enabled;
  }

  // Update method to be called in loop()
  void update() {
    if (!enabled) return;

    // All nodes broadcast beacons to build and maintain the mesh
    broadcastBeacon();

    if (mode == "sensor") {
      retryDiscoveryIfNeeded();
    }
  }
};

// Static instance pointer initialization (inline to avoid multiple definitions)
inline ESPNowManager* ESPNowManager::instance = nullptr;

#endif // ESPNOW_MANAGER_H
