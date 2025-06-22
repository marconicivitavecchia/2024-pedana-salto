#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <esp_wifi.h>
#include <lwip/sockets.h>
#include <lwip/tcp.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <vector>
#include <algorithm>
#include <map>

class WiFiManager {
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
        bool isHidden;           
        bool isMultiAP;          
        int apCount;             
        
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
    // CORE OPTIMIZATION
    // ===============================
    
    static bool optimizeEverything(const char* defaultPrimarySSID = nullptr, 
                                  const char* defaultPrimaryPassword = nullptr,
                                  const char* defaultBackupSSID = nullptr, 
                                  const char* defaultBackupPassword = nullptr) {
        Serial.println("=== Integrated WiFi Manager ===");
        
        // 1. Initialize filesystem
        if (!initializeFileSystem()) {
            Serial.println("⚠️ Filesystem initialization failed - using defaults only");
        }
        
        // 2. System optimization
        optimizeWiFi();
        optimizeCPU();
        
        // 3. Load credentials from filesystem or use defaults
        if (!loadCredentialsFromFileSystem()) {
            Serial.println("📁 Loading credentials from filesystem failed - using defaults");
            if (defaultPrimarySSID && defaultPrimaryPassword) {
                setupNetworks(defaultPrimarySSID, defaultPrimaryPassword, 
                             defaultBackupSSID, defaultBackupPassword);
            } else {
                Serial.println("❌ No default credentials provided!");
                return false;
            }
        } else {
            Serial.println("✅ Credentials loaded from filesystem");
        }
        
        if (_networks.empty()) {
            Serial.println("❌ No networks configured");
            return false;
        }
        
        // 4. Auto-detect network characteristics
        autoDetectNetworkTypes();
        
        // 5. Smart connection
        if (!connectIntelligent()) {
            return false;
        }
        
        Serial.println("✓ Integrated WiFi Manager ready!");
        return true;
    }
    
    // ===============================
    // FILESYSTEM MANAGEMENT
    // ===============================
    
    static bool initializeFileSystem() {
        if (!LittleFS.begin(true)) {
            Serial.println("❌ LittleFS Mount Failed");
            return false;
        }
        
        size_t totalBytes = LittleFS.totalBytes();
        size_t usedBytes = LittleFS.usedBytes();
        Serial.printf("✅ LittleFS: %d/%d bytes used\n", usedBytes, totalBytes);
        return true;
    }
    
    static bool loadCredentialsFromFileSystem() {
        const char* credentialsFile = "/wifi_credentials.txt";
        
        if (!LittleFS.exists(credentialsFile)) {
            Serial.println("📁 Credentials file not found");
            return false;
        }
        
        File file = LittleFS.open(credentialsFile, "r");
        if (!file) {
            Serial.println("❌ Failed to open credentials file");
            return false;
        }
        
        Serial.println("📖 Reading credentials from filesystem...");
        _networks.clear();
        
        String line;
        int lineNumber = 0;
        
        while (file.available()) {
            line = file.readStringUntil('\n');
            line.trim();
            lineNumber++;
            
            if (line.length() == 0 || line.startsWith("#")) {
                continue; // Skip empty lines and comments
            }
            
            // Parse line: SSID,PASSWORD
            int commaIndex = line.indexOf(',');
            if (commaIndex == -1) {
                Serial.printf("⚠️ Invalid format at line %d: %s\n", lineNumber, line.c_str());
                continue;
            }
            
            String ssid = line.substring(0, commaIndex);
            String password = line.substring(commaIndex + 1);
            
            ssid.trim();
            password.trim();
            
            if (ssid.length() == 0) {
                Serial.printf("⚠️ Empty SSID at line %d\n", lineNumber);
                continue;
            }
            
            bool isPreferred = (lineNumber == 1); // First network is preferred
            _networks.push_back(WiFiNetwork(ssid.c_str(), password.c_str(), isPreferred));
            
            Serial.printf("✅ Loaded %s: %s (%s)\n", 
                isPreferred ? "Primary" : "Backup", 
                ssid.c_str(), 
                password.length() > 0 ? "with password" : "open network");
        }
        
        file.close();
        
        if (_networks.empty()) {
            Serial.println("❌ No valid credentials found in file");
            return false;
        }
        
        Serial.printf("✅ Loaded %d network(s) from filesystem\n", _networks.size());
        return true;
    }
    
    static bool saveCredentialsToFileSystem(const String& primarySSID, const String& primaryPassword,
                                           const String& backupSSID = "", const String& backupPassword = "") {
        const char* credentialsFile = "/wifi_credentials.txt";
        
        File file = LittleFS.open(credentialsFile, "w");
        if (!file) {
            Serial.println("❌ Failed to create credentials file");
            return false;
        }
        
        // Write header comment
        file.println("# WiFi Credentials - ESP32 Integrated Manager");
        file.println("# Format: SSID,PASSWORD");
        file.println("# First line = Primary network, Second line = Backup network");
        file.println();
        
        // Write primary network
        file.printf("%s,%s\n", primarySSID.c_str(), primaryPassword.c_str());
        
        // Write backup network if provided
        if (backupSSID.length() > 0) {
            file.printf("%s,%s\n", backupSSID.c_str(), backupPassword.c_str());
        }
        
        file.close();
        
        Serial.println("✅ Credentials saved to filesystem");
        Serial.printf("   Primary: %s\n", primarySSID.c_str());
        if (backupSSID.length() > 0) {
            Serial.printf("   Backup: %s\n", backupSSID.c_str());
        }
        
        return true;
    }
	
	// AGGIUNTO: Metodo per ottenere le credenziali in formato JSON (per API)
	static String getCredentialsAsJson() {
	   DynamicJsonDocument doc(1024);
	   
	   // Status generale WiFi
	   doc["connected"] = (WiFi.status() == WL_CONNECTED);
	   doc["current_ssid"] = WiFi.isConnected() ? WiFi.SSID() : "";
	   doc["rssi"] = WiFi.isConnected() ? WiFi.RSSI() : 0;
	   doc["ip"] = WiFi.isConnected() ? WiFi.localIP().toString() : "";
	   doc["status"] = getStatusString(WiFi.status());
	   
	   // Informazioni sistema
	   doc["heap_free"] = ESP.getFreeHeap();
	   doc["uptime"] = millis();
	   
	   // Credenziali configurate
	   JsonArray networks = doc.createNestedArray("configured_networks");
	   
	   for (size_t i = 0; i < _networks.size(); i++) {
		   JsonObject network = networks.createNestedObject();
		   network["ssid"] = _networks[i].ssid;
		   network["is_preferred"] = _networks[i].isPreferred;
		   network["is_hidden"] = _networks[i].isHidden;
		   network["last_rssi"] = _networks[i].lastRSSI;
		   network["fail_count"] = _networks[i].failCount;
		   network["is_current"] = (_currentNetwork && _currentNetwork->ssid == _networks[i].ssid);
		   // Non includiamo la password per sicurezza
	   }
	   
	   String result;
	   serializeJson(doc, result);
	   return result;
	}
    
    // ===============================
    // DEBUG E DIAGNOSTICA AVANZATA
    // ===============================
    
    static void diagnoseNetworkIssues(const String& ssid) {
        Serial.printf("🔍 DIAGNOSTICA COMPLETA PER: %s\n", ssid.c_str());
        Serial.println("========================================");
        
        // 1. Info sistema
        Serial.printf("ESP32 Chip: %s\n", ESP.getChipModel());
        Serial.printf("CPU Freq: %d MHz\n", getCpuFrequencyMhz());
        Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
        
        // 2. Scan dettagliato
        Serial.println("\n📡 SCAN DETTAGLIATO:");
        WiFi.scanDelete();
        int n = WiFi.scanNetworks(false, true);
        
        bool found = false;
        for (int i = 0; i < n; i++) {
            if (WiFi.SSID(i) == ssid) {
                found = true;
                Serial.printf("  🎯 TROVATO: %s\n", ssid.c_str());
                Serial.printf("     BSSID: %s\n", WiFi.BSSIDstr(i).c_str());
                Serial.printf("     RSSI: %d dBm (%s)\n", WiFi.RSSI(i), 
                             WiFi.RSSI(i) > -50 ? "Eccellente" :
                             WiFi.RSSI(i) > -60 ? "Buono" :
                             WiFi.RSSI(i) > -70 ? "Discreto" :
                             WiFi.RSSI(i) > -80 ? "Debole" : "Molto debole");
                Serial.printf("     Canale: %d\n", WiFi.channel(i));
                Serial.printf("     Crittografia: %s\n", getSecurityString(WiFi.encryptionType(i)).c_str());
            }
        }
        
        if (!found) {
            Serial.printf("❌ Rete '%s' NON TROVATA!\n", ssid.c_str());
            Serial.println("Reti disponibili:");
            for (int i = 0; i < n; i++) {
                Serial.printf("  - %s (RSSI: %d)\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
            }
        }
        
        WiFi.scanDelete();
        Serial.println("========================================\n");
    }
    
    static bool connectWithDetailedDebug(WiFiNetwork& network) {
        Serial.printf("🔍 DEBUG: Connessione a %s\n", network.ssid.c_str());
        Serial.printf("   Password length: %d\n", network.password.length());
        
        // 1. RESET COMPLETO
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        delay(1000);
        WiFi.mode(WIFI_STA);
        delay(1000);
        
        // 2. SCAN PREVENTIVO
        Serial.println("🔍 Verifica presenza rete...");
        int n = WiFi.scanNetworks(false, true);
        bool networkFound = false;
        int bestRSSI = -100;
        int bestChannel = 0;
        String bestBSSID = "";
        
        for (int i = 0; i < n; i++) {
            if (WiFi.SSID(i) == network.ssid) {
                networkFound = true;
                int rssi = WiFi.RSSI(i);
                if (rssi > bestRSSI) {
                    bestRSSI = rssi;
                    bestChannel = WiFi.channel(i);
                    bestBSSID = WiFi.BSSIDstr(i);
                }
                Serial.printf("   Trovato AP: BSSID=%s, Canale=%d, RSSI=%d\n", 
                             WiFi.BSSIDstr(i).c_str(), WiFi.channel(i), rssi);
            }
        }
        
        if (!networkFound) {
            Serial.printf("❌ Rete '%s' non trovata nello scan!\n", network.ssid.c_str());
            WiFi.scanDelete();
            return false;
        }
        
        Serial.printf("✅ Rete trovata - Migliore: RSSI=%d, Canale=%d\n", bestRSSI, bestChannel);
        WiFi.scanDelete();
        
        // 3. CONNESSIONE CON PARAMETRI SPECIFICI
        if (bestChannel > 0 && bestBSSID.length() > 0) {
            Serial.printf("🎯 Connessione all'AP specifico: %s canale %d\n", bestBSSID.c_str(), bestChannel);
            uint8_t bssid[6];
            if (parseBSSID(bestBSSID, bssid)) {
                WiFi.begin(network.ssid.c_str(), network.password.c_str(), bestChannel, bssid, true);
            } else {
                WiFi.begin(network.ssid.c_str(), network.password.c_str());
            }
        } else {
            WiFi.begin(network.ssid.c_str(), network.password.c_str());
        }
        
        // 4. MONITORAGGIO DETTAGLIATO
        Serial.println("⏳ Tentativo di connessione...");
        unsigned long startTime = millis();
        int attempts = 0;
        
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) {
            wl_status_t status = WiFi.status();
            attempts++;
            
            if (attempts % 5 == 0) {
                Serial.printf("   Tentativo %d - Status: %s (%d)\n", 
                             attempts, getStatusString(status).c_str(), status);
                
                switch (status) {
                    case WL_CONNECT_FAILED:
                        Serial.println("   ❌ Autenticazione fallita - controlla password!");
                        return false;
                        
                    case WL_NO_SSID_AVAIL:
                        Serial.println("   ❌ SSID scomparso!");
                        break;
                        
                    case WL_IDLE_STATUS:
                        Serial.println("   ⏳ Connessione in corso...");
                        break;
                        
                    case WL_DISCONNECTED:
                        Serial.println("   🔄 Disconnesso - riprova...");
                        break;
                }
            }
            
            delay(1000);
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("✅ CONNESSO!\n");
            Serial.printf("   IP: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("   RSSI: %d dBm\n", WiFi.RSSI());
            Serial.printf("   Canale: %d\n", WiFi.channel());
            return true;
        } else {
            Serial.printf("❌ Connessione fallita dopo %d tentativi\n", attempts);
            Serial.printf("   Status finale: %s (%d)\n", getStatusString(WiFi.status()).c_str(), WiFi.status());
            return false;
        }
    }
    
    static bool fixRedmiConnection() {
        Serial.println("🔧 Applicando fix per Redmi...");
        
        // Fix specifici per telefoni Redmi/Xiaomi
        WiFi.setSleep(false);
        esp_wifi_set_ps(WIFI_PS_NONE);
        WiFi.setTxPower(WIFI_POWER_19_5dBm);
        
        // Configurazione ottimizzata per Redmi
        wifi_config_t conf;
        esp_wifi_get_config(WIFI_IF_STA, &conf);
        conf.sta.listen_interval = 1;
        conf.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
        esp_wifi_set_config(WIFI_IF_STA, &conf);
        
        return true;
    }
    
    static bool tryAlternativeConnectionMethods(WiFiNetwork& network) {
        Serial.println("🔄 Provo metodi alternativi...");
        
        // Metodo 1: Reset completo stack WiFi
        Serial.println("1️⃣ Reset completo WiFi stack");
        esp_wifi_stop();
        delay(1000);
        esp_wifi_start();
        delay(1000);
        
        if (connectWithDetailedDebug(network)) {
            Serial.println("✅ Metodo 1 riuscito!");
            return true;
        }
        
        // Metodo 2: Timeout esteso
        Serial.println("2️⃣ Timeout esteso");
        WiFi.disconnect();
        delay(2000);
        WiFi.begin(network.ssid.c_str(), network.password.c_str());
        
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < 45000) {
            delay(1000);
            if ((millis() - startTime) % 10000 == 0) {
                Serial.print(".");
            }
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n✅ Metodo 2 riuscito!");
            return true;
        }
        
        // Metodo 3: Reset canale
        Serial.println("3️⃣ Reset configurazione canale");
        WiFi.disconnect();
        delay(2000);
        
        esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
        delay(1000);
        
        WiFi.begin(network.ssid.c_str(), network.password.c_str());
        
        startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) {
            delay(1000);
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("✅ Metodo 3 riuscito!");
            return true;
        }
        
        Serial.println("❌ Tutti i metodi alternativi falliti");
        return false;
    }
    
    // Versione migliorata della connessione
    static bool connectToNetworkAdvanced(WiFiNetwork& network) {
        Serial.printf("🔄 CONNESSIONE AVANZATA a %s...\n", network.ssid.c_str());
        
        // 1. Diagnostica
        diagnoseNetworkIssues(network.ssid);
        
        // 2. Fix per Redmi se necessario
        if (network.ssid.indexOf("Redmi") >= 0 || network.ssid.indexOf("Xiaomi") >= 0) {
            fixRedmiConnection();
        }
        
        // 3. Tentativo con debug
        if (connectWithDetailedDebug(network)) {
            return true;
        }
        
        // 4. Metodi alternativi
        return tryAlternativeConnectionMethods(network);
    }
    
    // ===============================
    // CONTROLLO PERIODICO
    // ===============================
    
    static bool checkNetworkSwitch() {
        if (!_currentNetwork || WiFi.status() != WL_CONNECTED) {
            Serial.println("⚠️ Not connected - attempting reconnection");
            return connectIntelligent();
        }
        
        int currentRSSI = WiFi.RSSI();
        Serial.printf("🔍 Current network: %s (RSSI: %d dBm)\n", 
                      _currentNetwork->ssid.c_str(), currentRSSI);
        
        if (currentRSSI > -70) {
            Serial.println("✅ Signal strong, no switch needed");
            return true;
        }
        
        return true; // Semplificato per ora
    }
    
    // AGGIUNTO: Metodo mancante che veniva chiamato dalla macro
    static bool quickHealthCheck() {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("⚠️ WiFi disconnected - attempting reconnection");
            return connectIntelligent();
        }
        
        int rssi = WiFi.RSSI();
        if (rssi < -85) {
            Serial.printf("⚠️ Weak signal (%d dBm) - checking for better network\n", rssi);
            return checkNetworkSwitch();
        }
        
        Serial.printf("✅ WiFi healthy - RSSI: %d dBm\n", rssi);
        return true;
    }
    
    static bool fixWiFiUniversal() {
        Serial.println("🔧 Universal WiFi Fix...");
        
        wl_status_t status = WiFi.status();
        
        switch (status) {
            case WL_CONNECTED:
                if (WiFi.RSSI() < -85) {
                    Serial.println("⚠️ Very weak signal - searching for better AP");
                    return checkNetworkSwitch();
                }
                Serial.println("✅ WiFi is working properly");
                return true;
                
            case WL_CONNECTION_LOST:
            case WL_DISCONNECTED:
                Serial.println("🔄 Connection lost - attempting reconnection");
                return connectIntelligent();
                
            case WL_CONNECT_FAILED:
                Serial.println("❌ Connection failed - trying different network");
                if (_currentNetwork) {
                    _currentNetwork->failCount++;
                }
                return connectIntelligent();
                
            default:
                Serial.printf("⚠️ Unknown WiFi status: %d - forcing reconnection\n", status);
                WiFi.disconnect();
                delay(2000);
                return connectIntelligent();
        }
    }
    
    static void periodicHealthCheck() {
        static uint32_t lastCheck = 0;
        uint32_t now = millis();
        
        // Controllo ogni 2 minuti (120000 ms)
        if (now - lastCheck > 120000) {
            Serial.println("⏰ Periodic health check...");
            
            if (!checkNetworkSwitch()) {
                Serial.println("⚠️ Network switch failed - attempting universal fix");
                fixWiFiUniversal();
            }
            
            lastCheck = now;
        }
    }
    
    // ===============================
    // UTILITY METHODS
    // ===============================
    
    static String getStatusString(wl_status_t status) {
        switch (status) {
            case WL_CONNECTED: return "connected";
            case WL_NO_SSID_AVAIL: return "no_ssid";
            case WL_CONNECT_FAILED: return "connect_failed";
            case WL_CONNECTION_LOST: return "connection_lost";
            case WL_DISCONNECTED: return "disconnected";
            case WL_IDLE_STATUS: return "idle";
            default: return "unknown";
        }
    }
    
    static String getSecurityString(wifi_auth_mode_t auth) {
        switch (auth) {
            case WIFI_AUTH_OPEN: return "Open";
            case WIFI_AUTH_WEP: return "WEP";
            case WIFI_AUTH_WPA_PSK: return "WPA";
            case WIFI_AUTH_WPA2_PSK: return "WPA2";
            case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
            case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
            case WIFI_AUTH_WPA3_PSK: return "WPA3";
            default: return "Unknown";
        }
    }
    
    static int rssiToQuality(int rssi) {
        if (rssi >= -50) return 100;
        if (rssi >= -60) return 80;
        if (rssi >= -70) return 60;
        if (rssi >= -80) return 40;
        if (rssi >= -90) return 20;
        return 0;
    }
    
    // ===============================
    // CORE METHODS
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
            getCpuFrequencyMhz(), ESP.getFreeHeap());
    }
    
    static void setupNetworks(const char* primarySSID, const char* primaryPassword,
                             const char* backupSSID, const char* backupPassword) {
        _networks.clear();
        
        _networks.push_back(WiFiNetwork(primarySSID, primaryPassword, true));
        Serial.printf("✓ Primary network: %s\n", primarySSID);
        
        if (backupSSID && strlen(backupSSID) > 0) {
            _networks.push_back(WiFiNetwork(backupSSID, backupPassword, false));
            Serial.printf("✓ Backup network: %s\n", backupSSID);
        }
    }
    
    static void autoDetectNetworkTypes() {
        Serial.println("🔍 Auto-detecting network types...");
        
        WiFi.mode(WIFI_STA);
        WiFi.disconnect(true);
        delay(1000);
        
        int n = WiFi.scanNetworks(false, true);
        if (n <= 0) {
            Serial.println("❌ No networks found in scan");
            for (auto& network : _networks) {
                network.isHidden = true;
                network.lastRSSI = -75;
                Serial.printf("🔒 %s: Assumed HIDDEN (no scan results)\n", network.ssid.c_str());
            }
            return;
        }
        
        Serial.printf("📡 Found %d networks in scan\n", n);
        
        std::map<String, std::vector<APInfo>> networkMap;
        
        for (int i = 0; i < n; i++) {
            String ssid = WiFi.SSID(i);
            int rssi = WiFi.RSSI(i);
            int channel = WiFi.channel(i);
            String bssid = WiFi.BSSIDstr(i);
            wifi_auth_mode_t auth = WiFi.encryptionType(i);
            
            if (ssid.length() > 0) {
                APInfo ap = {rssi, channel, bssid, auth};
                networkMap[ssid].push_back(ap);
            }
        }
        
        for (auto& network : _networks) {
            if (networkMap.find(network.ssid) != networkMap.end()) {
                auto& aps = networkMap[network.ssid];
                network.isHidden = false;
                network.apCount = aps.size();
                network.isMultiAP = (aps.size() > 1);
                
                auto bestAP = std::max_element(aps.begin(), aps.end(),
                    [](const APInfo& a, const APInfo& b) {
                        return a.rssi < b.rssi;
                    });
                
                network.lastRSSI = bestAP->rssi;
                network.bestBSSID = bestAP->bssid;
                network.bestChannel = bestAP->channel;
                
                Serial.printf("👁️ %s: VISIBLE (%d APs, best RSSI: %d dBm) %s\n", 
                    network.ssid.c_str(), network.apCount, network.lastRSSI,
                    network.isMultiAP ? "[MULTI-AP]" : "");
            } else {
                network.isHidden = true;
                network.lastRSSI = -75;
                network.apCount = 0;
                network.isMultiAP = false;
                
                Serial.printf("🔒 %s: HIDDEN (estimated RSSI: %d dBm)\n", 
                    network.ssid.c_str(), network.lastRSSI);
            }
        }
        
        WiFi.scanDelete();
    }
    
    static bool connectIntelligent() {
        Serial.println("🧠 Intelligent connection...");
        
        if (_networks.empty()) {
            Serial.println("❌ No networks configured");
            return false;
        }
        
        std::sort(_networks.begin(), _networks.end(),
            [](const WiFiNetwork& a, const WiFiNetwork& b) {
                if (a.isPreferred != b.isPreferred) {
                    return a.isPreferred > b.isPreferred;
                }
                return a.lastRSSI > b.lastRSSI;
            });
        
        for (auto& network : _networks) {
            if (network.failCount >= MAX_FAIL_COUNT) {
                Serial.printf("⏭️ Skipping %s (too many failures: %d)\n", 
                    network.ssid.c_str(), network.failCount);
                continue;
            }
            
            if (connectToNetworkUniversal(network)) {
                _currentNetwork = &network;
                network.lastConnected = millis();
                network.failCount = 0;
                Serial.printf("✅ Connected to %s!\n", network.ssid.c_str());
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
    
    static bool connectToNetworkUniversal(WiFiNetwork& network) {
        Serial.printf("🔄 Connecting to %s...\n", network.ssid.c_str());
        
        if (network.isHidden) {
            WiFi.begin(network.ssid.c_str(), network.password.c_str());
        } else if (network.isMultiAP && network.bestBSSID.length() > 0) {
            uint8_t bssid[6];
            if (parseBSSID(network.bestBSSID, bssid)) {
                WiFi.begin(network.ssid.c_str(), network.password.c_str(), 
                          network.bestChannel, bssid, true);
            } else {
                WiFi.begin(network.ssid.c_str(), network.password.c_str());
            }
        } else {
            WiFi.begin(network.ssid.c_str(), network.password.c_str());
        }
        
        int maxAttempts = 20;
        if (network.isPreferred) maxAttempts += 10;
        if (network.isHidden) maxAttempts += 10;
        
        for (int i = 0; i < maxAttempts; i++) {
            wl_status_t status = WiFi.status();
            
            if (status == WL_CONNECTED) {
                if (network.isHidden) {
                    network.lastRSSI = WiFi.RSSI();
                    network.bestBSSID = WiFi.BSSIDstr();
                    network.bestChannel = WiFi.channel();
                }
                return true;
            }
            
            if (status == WL_CONNECT_FAILED) {
                Serial.println("❌ Wrong password?");
                return false;
            }
            
            if (status == WL_NO_SSID_AVAIL && !network.isHidden && i > 10) {
                Serial.println("❌ SSID not available");
                return false;
            }
            
            delay(1000);
            if (i % 5 == 4) Serial.print(".");
        }
        
        return false;
    }

private:
    static std::vector<WiFiNetwork> _networks;
    static WiFiNetwork* _currentNetwork;
    static const int MAX_FAIL_COUNT = 3;
    
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
};

// Static variable initialization
std::vector<WiFiManager::WiFiNetwork> WiFiManager::_networks;
WiFiManager::WiFiNetwork* WiFiManager::_currentNetwork = nullptr;

// ===============================
// MACRO DI INTEGRAZIONE
// ===============================

#define SETUP_INTEGRATED_WIFI() \
    WiFiManager::optimizeEverything()

#define SETUP_INTEGRATED_WIFI_FALLBACK(primarySSID, primaryPass, backupSSID, backupPass) \
    WiFiManager::optimizeEverything(primarySSID, primaryPass, backupSSID, backupPass)

#define WIFI_HEALTH_CHECK() \
    WiFiManager::quickHealthCheck()

#define CHECK_NETWORK_SWITCH() \
    WiFiManager::checkNetworkSwitch()

#define FIX_WIFI_UNIVERSAL() \
    WiFiManager::fixWiFiUniversal()

#define PERIODIC_WIFI_CHECK() \
    WiFiManager::periodicHealthCheck()

// ===============================
// MACRO DI DEBUG
// ===============================

#define DIAGNOSE_NETWORK(ssid) \
    WiFiManager::diagnoseNetworkIssues(ssid)

#define DEBUG_WIFI_CONNECTION(network) \
    WiFiManager::connectToNetworkAdvanced(network)

#define FIX_REDMI_ISSUES() \
    WiFiManager::fixRedmiConnection()

#define CONNECT_WITH_DEBUG(ssid, password) \
    do { \
        WiFiManager::WiFiNetwork debugNet(ssid, password, true); \
        WiFiManager::connectToNetworkAdvanced(debugNet); \
    } while(0)

#endif // WIFI_MANAGER_H