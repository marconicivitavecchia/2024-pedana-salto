#ifndef GENERIC_WEBSOCKET_OPTIMIZER_H
#define GENERIC_WEBSOCKET_OPTIMIZER_H

#include <WiFi.h>
#include <esp_wifi.h>
#include <lwip/sockets.h>
#include <lwip/tcp.h>
#include <vector>
#include <algorithm>
#include <map>

class GenericWebSocketOptimizer {
public:
    
    struct WiFiNetwork {
        String ssid;
        String password;
        int lastRSSI;
        uint32_t lastConnected;
        uint32_t failCount;
        bool isPreferred;
        String bestBSSID;        
        int bestChannel;         
        bool isHidden;           // Auto-rilevato
        bool isMultiAP;          // Ha più Access Point
        int apCount;             // Numero di AP trovati
        
        WiFiNetwork() : lastRSSI(-100), lastConnected(0), failCount(0), isPreferred(false), 
                       bestChannel(0), isHidden(false), isMultiAP(false), apCount(0) {}
        WiFiNetwork(const char* s, const char* p, bool preferred = false) 
            : ssid(s), password(p), lastRSSI(-100), lastConnected(0), failCount(0), 
              isPreferred(preferred), bestChannel(0), isHidden(false), isMultiAP(false), apCount(0) {}
    };
    
    struct APInfo {
        int rssi;
        int channel;
        String bssid;
        wifi_auth_mode_t auth;
    };
    
    // ===============================
    // OTTIMIZZAZIONE GENERICA UNIVERSALE
    // ===============================
    
    static bool optimizeEverything(const char* primarySSID, const char* primaryPassword,
                                  const char* backupSSID = nullptr, const char* backupPassword = nullptr) {
        Serial.println("=== Generic WebSocket Optimization (Auto-detect) ===");
        
        // 1. Ottimizza sistema
        optimizeWiFi();
        optimizeCPU();
        
        // 2. Setup reti
        setupNetworks(primarySSID, primaryPassword, backupSSID, backupPassword);
        
        // 3. Auto-rileva caratteristiche reti
        autoDetectNetworkTypes();
        
        // 4. Connetti intelligentemente
        if (!connectIntelligent()) {
            return false;
        }
        
        Serial.println("✓ Generic optimization complete!");
        return true;
    }
    
    // ===============================
    // OTTIMIZZAZIONI SISTEMA
    // ===============================
    
    static void optimizeWiFi() {
        Serial.println("--- WiFi Optimization ---");
        WiFi.setSleep(false);
        esp_wifi_set_ps(WIFI_PS_NONE);
        WiFi.setTxPower(WIFI_POWER_19_5dBm);
        WiFi.setAutoReconnect(true);
        Serial.println("✓ WiFi optimized");
    }
    
    static void optimizeCPU() {
        Serial.println("--- CPU Optimization ---");
        setCpuFrequencyMhz(240);
        Serial.printf("✓ CPU: %d MHz, Heap: %d bytes\n", 
            getCpuFrequencyMhz(), esp_get_free_heap_size());
    }
    
    // ===============================
    // SETUP RETI GENERICHE
    // ===============================
    
    static void setupNetworks(const char* primarySSID, const char* primaryPassword,
                             const char* backupSSID, const char* backupPassword) {
        _networks.clear();
        
        // Aggiungi rete principale
        _networks.push_back(WiFiNetwork(primarySSID, primaryPassword, true));
        Serial.printf("✓ Primary network: %s\n", primarySSID);
        
        // Aggiungi rete backup se fornita
        if (backupSSID && strlen(backupSSID) > 0) {
            _networks.push_back(WiFiNetwork(backupSSID, backupPassword, false));
            Serial.printf("✓ Backup network: %s\n", backupSSID);
        }
    }
    
    // ===============================
    // AUTO-RILEVAMENTO TIPI DI RETE
    // ===============================
    
    static void autoDetectNetworkTypes() {
        Serial.println("🔍 Auto-detecting network types...");
        
        // Scansione completa
        WiFi.mode(WIFI_STA);
        WiFi.disconnect(true);
        delay(1000);
        
        int n = WiFi.scanNetworks(false, true); // include hidden
        if (n <= 0) {
            Serial.println("❌ No networks found in scan");
            // Assume tutte nascoste
            for (auto& network : _networks) {
                network.isHidden = true;
                network.lastRSSI = -75; // Stima conservativa
                Serial.printf("🔒 %s: Assumed HIDDEN (no scan results)\n", network.ssid.c_str());
            }
            return;
        }
        
        Serial.printf("📡 Found %d networks in scan\n", n);
        
        // Raggruppa per SSID
        std::map<String, std::vector<APInfo>> networkMap;
        
        for (int i = 0; i < n; i++) {
            String ssid = WiFi.SSID(i);
            int rssi = WiFi.RSSI(i);
            int channel = WiFi.channel(i);
            String bssid = WiFi.BSSIDstr(i);
            wifi_auth_mode_t auth = WiFi.encryptionType(i);
            
            if (ssid.length() > 0) { // Solo reti visibili
                APInfo ap = {rssi, channel, bssid, auth};
                networkMap[ssid].push_back(ap);
            }
        }
        
        // Analizza ogni rete configurata
        for (auto& network : _networks) {
            if (networkMap.find(network.ssid) != networkMap.end()) {
                // Rete VISIBILE
                auto& aps = networkMap[network.ssid];
                network.isHidden = false;
                network.apCount = aps.size();
                network.isMultiAP = (aps.size() > 1);
                
                // Trova il miglior AP
                auto bestAP = std::max_element(aps.begin(), aps.end(),
                    [](const APInfo& a, const APInfo& b) {
                        return a.rssi < b.rssi;
                    });
                
                network.lastRSSI = bestAP->rssi;
                network.bestBSSID = bestAP->bssid;
                network.bestChannel = bestAP->channel;
                
                Serial.printf("👁️  %s: VISIBLE (%d APs, best RSSI: %d dBm) %s\n", 
                    network.ssid.c_str(), network.apCount, network.lastRSSI,
                    network.isMultiAP ? "[MULTI-AP]" : "");
                
                if (network.isMultiAP) {
                    Serial.printf("   APs found: ");
                    for (const auto& ap : aps) {
                        Serial.printf("%s(%d) ", ap.bssid.c_str(), ap.rssi);
                    }
                    Serial.println();
                }
            } else {
                // Rete NASCOSTA (non trovata in scan)
                network.isHidden = true;
                network.lastRSSI = -75; // Stima conservativa
                network.apCount = 0;
                network.isMultiAP = false;
                
                Serial.printf("🔒 %s: HIDDEN (estimated RSSI: %d dBm)\n", 
                    network.ssid.c_str(), network.lastRSSI);
            }
        }
        
        WiFi.scanDelete();
    }
    
    // ===============================
    // CONNESSIONE INTELLIGENTE UNIVERSALE
    // ===============================
    
    static bool connectIntelligent() {
        Serial.println("🧠 Intelligent connection (auto-adapt)...");
        
        if (_networks.empty()) {
            Serial.println("❌ No networks configured");
            return false;
        }
        
        // Ordina per priorità (preferite prime, poi migliore segnale)
        std::sort(_networks.begin(), _networks.end(),
            [](const WiFiNetwork& a, const WiFiNetwork& b) {
                if (a.isPreferred != b.isPreferred) {
                    return a.isPreferred > b.isPreferred;
                }
                return a.lastRSSI > b.lastRSSI;
            });
        
        Serial.println("🎯 Connection order:");
        for (size_t i = 0; i < _networks.size(); i++) {
            const auto& net = _networks[i];
            Serial.printf("  %d. %s (%s, %d dBm, %d fails) %s%s\n", 
                i+1, net.ssid.c_str(),
                net.isPreferred ? "Primary" : "Backup",
                net.lastRSSI, net.failCount,
                net.isHidden ? "[HIDDEN] " : "",
                net.isMultiAP ? "[MULTI-AP]" : "");
        }
        
        // Prova a connettersi in ordine
        for (auto& network : _networks) {
            if (network.failCount >= MAX_FAIL_COUNT) {
                Serial.printf("⏭️  Skipping %s (too many failures: %d)\n", 
                    network.ssid.c_str(), network.failCount);
                continue;
            }
            
            if (connectToNetworkUniversal(network)) {
                _currentNetwork = &network;
                network.lastConnected = millis();
                network.failCount = 0;
                Serial.printf("✅ Connected to %s!\n", network.ssid.c_str());
                printConnectionInfo();
                return true;
            } else {
                network.failCount++;
                Serial.printf("❌ Failed to connect to %s (failures: %d)\n", 
                    network.ssid.c_str(), network.failCount);
            }
        }
        
        Serial.println("❌ All networks failed");
        return false;
    }
    
    // ===============================
    // CONNESSIONE UNIVERSALE (HIDDEN/VISIBLE/MULTI-AP)
    // ===============================
    
    static bool connectToNetworkUniversal(WiFiNetwork& network) {
        String networkType = "";
        if (network.isHidden) networkType += "HIDDEN ";
        if (network.isMultiAP) networkType += "MULTI-AP ";
        if (networkType.isEmpty()) networkType = "STANDARD ";
        
        Serial.printf("🔄 Connecting to %s %s...\n", 
            networkType.c_str(), network.ssid.c_str());
        
        // Strategia di connessione basata sul tipo
        if (network.isHidden) {
            // Rete nascosta - connessione semplice
            Serial.println("   Strategy: Hidden network connection");
            WiFi.begin(network.ssid.c_str(), network.password.c_str());
        } else if (network.isMultiAP && network.bestBSSID.length() > 0) {
            // Multi-AP visibile - connessione al BSSID specifico
            Serial.printf("   Strategy: Target best AP %s (RSSI: %d)\n", 
                network.bestBSSID.c_str(), network.lastRSSI);
            
            uint8_t bssid[6];
            if (parseBSSID(network.bestBSSID, bssid)) {
                WiFi.begin(network.ssid.c_str(), network.password.c_str(), 
                          network.bestChannel, bssid, true);
            } else {
                Serial.println("   BSSID parse failed - using standard connection");
                WiFi.begin(network.ssid.c_str(), network.password.c_str());
            }
        } else {
            // Rete standard
            Serial.println("   Strategy: Standard connection");
            WiFi.begin(network.ssid.c_str(), network.password.c_str());
        }
        
        // Timeout dinamico basato sul tipo di rete
        int maxAttempts = 20; // Base
        if (network.isPreferred) maxAttempts += 20;  // +20 per principale
        if (network.isHidden) maxAttempts += 20;     // +20 per nascosta
        if (network.isMultiAP) maxAttempts += 10;    // +10 per multi-AP
        
        Serial.printf("   Timeout: %d seconds\n", maxAttempts);
        
        // Attesa connessione con progress
        for (int i = 0; i < maxAttempts; i++) {
            wl_status_t status = WiFi.status();
            
            if (status == WL_CONNECTED) {
                Serial.printf("\n✅ Connected! (%d/%d attempts)\n", i+1, maxAttempts);
                
                // Aggiorna info per reti nascoste
                if (network.isHidden) {
                    network.lastRSSI = WiFi.RSSI();
                    network.bestBSSID = WiFi.BSSIDstr();
                    network.bestChannel = WiFi.channel();
                    Serial.println("   Updated hidden network info");
                }
                
                Serial.printf("   Final: BSSID=%s, RSSI=%d, Channel=%d\n",
                    WiFi.BSSIDstr().c_str(), WiFi.RSSI(), WiFi.channel());
                
                return true;
            }
            
            // Check errori fatali
            if (status == WL_CONNECT_FAILED) {
                Serial.printf("\n❌ Connection failed - wrong password?\n");
                return false;
            }
            
            if (status == WL_NO_SSID_AVAIL && !network.isHidden && i > 10) {
                Serial.printf("\n❌ SSID not available\n");
                return false;
            }
            
            delay(1000);
            if (i % 10 == 9) {
                Serial.printf("\n   Progress: %d/%d (Status: %d)", i+1, maxAttempts, status);
            } else {
                Serial.print(".");
            }
        }
        
        Serial.printf("\n❌ Connection timeout (%d seconds)\n", maxAttempts);
        return false;
    }
    
    // ===============================
    // OTTIMIZZAZIONE SOCKET
    // ===============================
    
    static bool optimizeSocket(int sockfd) {
        Serial.printf("⚡ Optimizing socket %d...\n", sockfd);
        
        // TCP_NODELAY per latenza
        int flag = 1;
        setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
        
        // Buffer grandi
        int bufSize = 32768;
        setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &bufSize, sizeof(bufSize));
        setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &bufSize, sizeof(bufSize));
        
        // Keep-alive
        flag = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag));
        
        // Timeout
        struct timeval timeout = {5, 0};
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        
        Serial.println("✓ Socket optimized");
        return true;
    }
    
    // ===============================
    // SERVER CONFIG OTTIMIZZATO
    // ===============================
    
    static httpd_config_t getOptimalServerConfig(uint16_t port) {
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.server_port = port;
        config.max_open_sockets = 6;
        config.stack_size = 12288;
        config.task_priority = 20;
        config.core_id = 1;
        config.send_wait_timeout = 5;
        config.recv_wait_timeout = 5;
        config.lru_purge_enable = true;
        
        Serial.printf("✓ Server config: port=%d, priority=%d\n", 
            config.server_port, config.task_priority);
        return config;
    }
    
    // ===============================
    // CONTROLLI E MANUTENZIONE
    // ===============================
    
    static void quickHealthCheck() {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("⚠️  WiFi disconnected - attempting reconnection");
            connectIntelligent();
            return;
        }
        
        int rssi = WiFi.RSSI();
        if (rssi < -80) {
            Serial.printf("⚠️  Weak signal: %d dBm\n", rssi);
        }
        
        size_t freeHeap = esp_get_free_heap_size();
        if (freeHeap < 15000) {
            Serial.printf("⚠️  Low memory: %d bytes\n", freeHeap);
        }
    }
    
    static void printQuickStats() {
        String networkInfo = getCurrentNetworkInfo();
        Serial.printf("Network: %s, RSSI: %d dBm, Heap: %d bytes\n",
            networkInfo.c_str(), WiFi.RSSI(), esp_get_free_heap_size());
    }
    
    static bool fixWiFiIssues() {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("🔧 WiFi disconnected - attempting intelligent reconnection");
            return connectIntelligent();
        }
        
        int rssi = WiFi.RSSI();
        if (rssi < -85) {
            Serial.printf("🔧 Very weak signal (%d dBm)\n", rssi);
            WiFi.setTxPower(WIFI_POWER_19_5dBm);
            
            if (_networks.size() > 1) {
                Serial.println("🔧 Checking for better networks...");
                checkAndSwitchNetwork();
            }
        }
        
        return true;
    }
    
    static void checkAndSwitchNetwork() {
        if (_networks.size() < 2 || WiFi.status() != WL_CONNECTED) {
            return;
        }
        
        int currentRSSI = WiFi.RSSI();
        if (currentRSSI > SWITCH_THRESHOLD) {
            return; // Segnale ancora buono
        }
        
        Serial.printf("📶 Weak signal (%d dBm) - scanning for alternatives\n", currentRSSI);
        
        // Re-detect network types per aggiornare RSSI
        autoDetectNetworkTypes();
        
        // Trova alternativa migliore
        WiFiNetwork* bestAlternative = nullptr;
        for (auto& network : _networks) {
            if (network.ssid != WiFi.SSID() && 
                network.lastRSSI > currentRSSI + SWITCH_HYSTERESIS &&
                network.failCount < MAX_FAIL_COUNT) {
                bestAlternative = &network;
                break;
            }
        }
        
        if (bestAlternative) {
            Serial.printf("🔄 Switching to better network: %s (%d dBm)\n", 
                bestAlternative->ssid.c_str(), bestAlternative->lastRSSI);
            
            WiFi.disconnect();
            delay(1000);
            
            if (connectToNetworkUniversal(*bestAlternative)) {
                _currentNetwork = bestAlternative;
                bestAlternative->lastConnected = millis();
                Serial.printf("✅ Successfully switched to %s\n", bestAlternative->ssid.c_str());
            } else {
                Serial.println("❌ Switch failed - attempting reconnection");
                connectIntelligent();
            }
        }
    }
    
    static void freeMemoryIfLow() {
        size_t freeHeap = esp_get_free_heap_size();
        if (freeHeap < 15000) {
            Serial.printf("🧹 Low memory (%d bytes) - cleanup\n", freeHeap);
            WiFi.scanDelete();
            for (int i = 0; i < 10; i++) {
                yield();
                delay(1);
            }
            Serial.printf("   After cleanup: %d bytes\n", esp_get_free_heap_size());
        }
    }
    
    // ===============================
    // DEBUG UNIVERSALE
    // ===============================
    
    static void debugAnyNetwork(const char* ssid, const char* password) {
        Serial.println("=== UNIVERSAL NETWORK DEBUG ===");
        Serial.printf("Testing network: '%s'\n", ssid);
        
        WiFi.mode(WIFI_STA);
        WiFi.disconnect(true);
        delay(1000);
        
        // Crea rete temporanea per test
        WiFiNetwork testNet(ssid, password, true);
        
        // Auto-rileva tipo
        _networks.clear();
        _networks.push_back(testNet);
        autoDetectNetworkTypes();
        
        // Testa connessione
        if (connectToNetworkUniversal(_networks[0])) {
            Serial.println("✅ Test successful!");
            printConnectionInfo();
        } else {
            Serial.println("❌ Test failed!");
        }
    }

private:
    static std::vector<WiFiNetwork> _networks;
    static WiFiNetwork* _currentNetwork;
    static const int MAX_FAIL_COUNT = 3;
    static const int SWITCH_THRESHOLD = -75;
    static const int SWITCH_HYSTERESIS = 10;
    
    static bool parseBSSID(const String& bssidStr, uint8_t* bssid) {
        if (bssidStr.length() != 17) return false;
        int values[6];
        if (sscanf(bssidStr.c_str(), "%x:%x:%x:%x:%x:%x",
                   &values[0], &values[1], &values[2], 
                   &values[3], &values[4], &values[5]) == 6) {
            for (int i = 0; i < 6; i++) {
                bssid[i] = (uint8_t)values[i];
            }
            return true;
        }
        return false;
    }
    
    static String getCurrentNetworkInfo() {
        if (WiFi.status() != WL_CONNECTED) {
            return "Disconnected";
        }
        
        String info = WiFi.SSID();
        info += " (";
        info += WiFi.RSSI();
        info += " dBm)";
        
        if (_currentNetwork) {
            if (_currentNetwork->isPreferred) info += " [Primary]";
            else info += " [Backup]";
            
            if (_currentNetwork->isHidden) info += " [Hidden]";
            if (_currentNetwork->isMultiAP) info += " [Multi-AP]";
        }
        
        return info;
    }
    
    static void printConnectionInfo() {
        Serial.println("=== Connection Info ===");
        Serial.printf("Network: %s\n", getCurrentNetworkInfo().c_str());
        Serial.printf("BSSID: %s\n", WiFi.BSSIDstr().c_str());
        Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
        Serial.printf("Channel: %d\n", WiFi.channel());
        Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
        Serial.println("======================");
    }
};

// Inizializzazione variabili statiche
std::vector<GenericWebSocketOptimizer::WiFiNetwork> GenericWebSocketOptimizer::_networks;
GenericWebSocketOptimizer::WiFiNetwork* GenericWebSocketOptimizer::_currentNetwork = nullptr;

// ===============================
// MACRO UNIVERSALI
// ===============================

// Setup universale (auto-detect tutto)
#define OPTIMIZE_WEBSOCKET_UNIVERSAL(primarySSID, primaryPass, backupSSID, backupPass) \
    GenericWebSocketOptimizer::optimizeEverything(primarySSID, primaryPass, backupSSID, backupPass)

// Setup singola rete
#define OPTIMIZE_WEBSOCKET_SINGLE(ssid, password) \
    GenericWebSocketOptimizer::optimizeEverything(ssid, password)

// Debug universale
#define DEBUG_ANY_NETWORK(ssid, password) \
    GenericWebSocketOptimizer::debugAnyNetwork(ssid, password)

// Ottimizza socket
#define OPTIMIZE_SOCKET_UNIVERSAL(sockfd) \
    GenericWebSocketOptimizer::optimizeSocket(sockfd)

// Controlli universali
#define UNIVERSAL_HEALTH_CHECK() \
    GenericWebSocketOptimizer::quickHealthCheck()

#define UNIVERSAL_NETWORK_SWITCH() \
    GenericWebSocketOptimizer::checkAndSwitchNetwork()

#define UNIVERSAL_STATS() \
    GenericWebSocketOptimizer::printQuickStats()

#define FIX_WIFI_UNIVERSAL() \
    GenericWebSocketOptimizer::fixWiFiIssues()

#endif // GENERIC_WEBSOCKET_OPTIMIZER_H

/*
===============================
ESEMPIO DI USO UNIVERSALE:
===============================

#include "GenericWebSocketOptimizer.h"
#include "DualWebSocket.h"

DualWebSocket ws;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    // *** OTTIMIZZAZIONE UNIVERSALE ***
    // Rileva automaticamente se le reti sono nascoste, multi-AP, ecc.
    OPTIMIZE_WEBSOCKET_UNIVERSAL(
        "ResteQualsiasi", "password123",     // Principale (auto-detect)
        "AltroWiFi", "password456"           // Backup (auto-detect)
    );
    
    // Oppure singola rete
    // OPTIMIZE_WEBSOCKET_SINGLE("MiaRete", "password");
    
    ws.begin();
    
    // Ottimizza socket automaticamente
    ws.onDataEvent([](WSEventType type, WebSocketServer::WSClient* client, 
                     uint8_t* data, size_t len, void* arg) {
        if (type == WS_EVT_CONNECT) {
            OPTIMIZE_SOCKET_UNIVERSAL(client->id);
        }
    });
}

void loop() {
    // Il tuo codice normale
    if (ws.hasDataClients()) {
        String data = "{\"value\":" + String(analogRead(A0)) + "}";
        ws.sendDataAsync(data.c_str(), data.length());
    }
    
    // Controlli automatici universali
    static uint32_t lastCheck = 0;
    if (millis() - lastCheck > 30000) {
        UNIVERSAL_HEALTH_CHECK();
        lastCheck = millis();
    }
    
    static uint32_t lastSwitch = 0;
    if (millis() - lastSwitch > 120000) {
        UNIVERSAL_NETWORK_SWITCH();
        lastSwitch = millis();
    }
    
    static uint32_t lastStats = 0;
    if (millis() - lastStats > 300000) {
        UNIVERSAL_STATS();
        GenericWebSocketOptimizer::freeMemoryIfLow();
        lastStats = millis();
    }
    
    delay(10);
}

===============================
CARATTERISTICHE UNIVERSALI:
===============================

✅ Auto-rileva reti nascoste
✅ Auto-rileva reti multi-AP  
✅ Gestione intelligente per ogni tipo
✅ Timeout dinamici basati sul tipo
✅ Strategia di connessione ottimale
✅ Switch automatico intelligente
✅ Zero configurazione manuale
✅ Compatibilità totale
✅ Debug universale

===============================
*/