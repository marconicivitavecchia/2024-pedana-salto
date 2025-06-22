// ===============================
// HANDLER WIFI ULTRA-SEMPLIFICATI
// ===============================
/*
GET 	/api/wifi/config 	- Ottieni configurazione
POST 	/api/wifi/config 	- Salva configurazione
DELETE 	/api/wifi/config 	- Reset configurazione
OPTIONS /api/wifi/config 	- CORS preflight
POST 	/api/wifi/test 		- Testa connessione
OPTIONS /api/wifi/test 		- CORS preflight
GET 	/api/wifi/scan 		- Scan reti disponibili
GET 	/api/system/status 	- Status sistema
*/
#ifndef HANDLERS_H
#define HANDLERS_H

#include <ArduinoJson.h>

static const char* TAG = "WiFiAPI";

// API 1: GET /api/wifi/config
esp_err_t apiWiFiConfigGet(httpd_req_t *req) {
    ESP32WebServer* server = (ESP32WebServer*)req->user_ctx;
    server->currentRequest = req;
    
    // Fix: Use the existing getCredentialsAsJson method that returns a String
    String response = WiFiManager::getCredentialsAsJson();
    
    server->sendJSON(response);
    server->currentRequest = nullptr;
    return ESP_OK;
}

// API 2: POST /api/wifi/config
esp_err_t apiWiFiConfigPost(httpd_req_t *req) {
    ESP32WebServer* server = (ESP32WebServer*)req->user_ctx;
    server->currentRequest = req;
    
    // CORS preflight
    if (req->method == HTTP_OPTIONS) {
        server->addAPIHeaders();
        server->currentRequest = nullptr;
        return httpd_resp_send(req, NULL, 0);
    }
    
    String json = server->readJSONBody();
    if (json.length() == 0) {
        server->sendJSONError("Empty request body");
        server->currentRequest = nullptr;
        return ESP_OK;
    }
    
    JsonDocument doc;  // Fix: Use JsonDocument instead of DynamicJsonDocument
    if (deserializeJson(doc, json)) {
        server->sendJSONError("Invalid JSON");
        server->currentRequest = nullptr;
        return ESP_OK;
    }
    
    String primarySSID = doc["primarySSID"] | "";
    String primaryPassword = doc["primaryPassword"] | "";
    String backupSSID = doc["backupSSID"] | "";
    String backupPassword = doc["backupPassword"] | "";
    bool autoConnect = doc["autoConnect"] | true;
    
    if (primarySSID.length() == 0) {
        server->sendJSONError("Primary SSID required");
        server->currentRequest = nullptr;
        return ESP_OK;
    }
    
    if (primarySSID.length() > 32) {
        server->sendJSONError("SSID too long (max 32 chars)");
        server->currentRequest = nullptr;
        return ESP_OK;
    }
    
    // Fix: Use the correct method names from WiFiManager
    bool success = WiFiManager::saveCredentialsToFileSystem(primarySSID, primaryPassword, backupSSID, backupPassword);
    
    // If you want to connect immediately after saving:
    if (success) {
        success = WiFiManager::connectIntelligent();
    }
    
    JsonDocument response;
    response["success"] = success;
    response["connecting"] = success;
    response["message"] = success ? "Configuration saved and connecting" : "Failed to save configuration";
    
    String responseStr;
    serializeJson(response, responseStr);
    server->sendJSON(responseStr, success ? 200 : 500);
    
    server->currentRequest = nullptr;
    return ESP_OK;
}

// API 3: DELETE /api/wifi/config
esp_err_t apiWiFiConfigDelete(httpd_req_t *req) {
    ESP32WebServer* server = (ESP32WebServer*)req->user_ctx;
    server->currentRequest = req;
    
    if (LittleFS.exists("/wifi_credentials.txt")) {
        LittleFS.remove("/wifi_credentials.txt");
    }
    
    // Also clear any in-memory credentials and reload
    WiFiManager::loadCredentialsFromFileSystem();
    
    server->sendJSONSuccess("WiFi configuration reset");
    server->currentRequest = nullptr;
    return ESP_OK;
}

// API 4: POST /api/wifi/test
esp_err_t apiWiFiTest(httpd_req_t *req) {
    ESP32WebServer* server = (ESP32WebServer*)req->user_ctx;
    server->currentRequest = req;
    
    if (req->method == HTTP_OPTIONS) {
        server->addAPIHeaders();
        server->currentRequest = nullptr;
        return httpd_resp_send(req, NULL, 0);
    }
    
    String json = server->readJSONBody();
    if (json.length() == 0) {
        server->sendJSONError("Empty request body");
        server->currentRequest = nullptr;
        return ESP_OK;
    }
    
    JsonDocument doc;  // Fix: Use JsonDocument
    if (deserializeJson(doc, json)) {
        server->sendJSONError("Invalid JSON");
        server->currentRequest = nullptr;
        return ESP_OK;
    }
    
    String ssid = doc["ssid"] | "";
    String password = doc["password"] | "";
    
    if (ssid.length() == 0) {
        server->sendJSONError("SSID required");
        server->currentRequest = nullptr;
        return ESP_OK;
    }
    
    // Simplified test connection approach
    WiFi.disconnect();
    delay(1000);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    unsigned long startTime = millis();
    bool success = false;
    while (millis() - startTime < 15000) {
        if (WiFi.status() == WL_CONNECTED) {
            success = true;
            break;
        }
        delay(500);
    }
    
    // Disconnect test connection and restore original
    WiFi.disconnect();
    delay(1000);
    WiFiManager::connectIntelligent();
    
    JsonDocument response;
    response["success"] = success;
    response["message"] = success ? "Connection test successful" : "Connection test failed";
    response["ssid"] = ssid;
    
    String responseStr;
    serializeJson(response, responseStr);
    server->sendJSON(responseStr, success ? 200 : 400);
    
    server->currentRequest = nullptr;
    return ESP_OK;
}

// API 5: GET /api/system/status
esp_err_t apiSystemStatus(httpd_req_t *req) {
    ESP32WebServer* server = (ESP32WebServer*)req->user_ctx;
    server->currentRequest = req;
    
    JsonDocument status;
    
    // Fix: Build the status JSON manually since getCurrentStatus doesn't exist
    status["wifi"]["connected"] = WiFi.status() == WL_CONNECTED;
    status["wifi"]["ssid"] = WiFi.SSID();
    status["wifi"]["ip"] = WiFi.localIP().toString();
    status["wifi"]["rssi"] = WiFi.RSSI();
    status["wifi"]["mac"] = WiFi.macAddress();
    
    status["system"]["heap"] = ESP.getFreeHeap();
    status["system"]["uptime"] = millis();
    status["system"]["chipModel"] = ESP.getChipModel();
    status["system"]["flashSize"] = ESP.getFlashChipSize();
    
    // Add server info
    status["server"]["uptime"] = millis();
    
    String response;
    serializeJson(status, response);
    
    // Cache per 5 secondi
    httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=5");
    server->sendJSON(response);
    
    server->currentRequest = nullptr;
    return ESP_OK;
}

// API 6: GET /api/wifi/scan
esp_err_t apiWiFiScan(httpd_req_t *req) {
    ESP32WebServer* server = (ESP32WebServer*)req->user_ctx;
    server->currentRequest = req;
    
    JsonDocument networks;
    
    // Fix: Implement WiFi scan manually since scanAndGetNetworks doesn't exist
    int networkCount = WiFi.scanNetworks();
    
    if (networkCount == 0) {
        networks["networks"] = JsonArray();
        networks["count"] = 0;
    } else {
        JsonArray networkArray = networks["networks"].to<JsonArray>();
        
        for (int i = 0; i < networkCount; i++) {
            JsonObject network = networkArray.add<JsonObject>();
            network["ssid"] = WiFi.SSID(i);
            network["rssi"] = WiFi.RSSI(i);
            network["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open" : "encrypted";
            network["channel"] = WiFi.channel(i);
        }
        networks["count"] = networkCount;
    }
    
    String response;
    serializeJson(networks, response);
    
    server->sendJSON(response);
    server->currentRequest = nullptr;
    return ESP_OK;
}

// ===============================
// SETUP FINALE
// ===============================
void setupWiFiAPI(ESP32WebServer& webServer) {
    Serial.println("[WiFi API] Registering endpoints...");
    
    webServer.addHandler("/api/wifi/config", HTTP_GET, apiWiFiConfigGet);
    webServer.addHandler("/api/wifi/config", HTTP_POST, apiWiFiConfigPost);
    webServer.addHandler("/api/wifi/config", HTTP_DELETE, apiWiFiConfigDelete);
    webServer.addHandler("/api/wifi/config", HTTP_OPTIONS, apiWiFiConfigPost);
    
    webServer.addHandler("/api/wifi/test", HTTP_POST, apiWiFiTest);
    webServer.addHandler("/api/wifi/test", HTTP_OPTIONS, apiWiFiTest);
    webServer.addHandler("/api/wifi/scan", HTTP_GET, apiWiFiScan);
    
    webServer.addHandler("/api/system/status", HTTP_GET, apiSystemStatus);
    
    Serial.println("[WiFi API] Ready - 6 endpoints registered");
}

// ===============================
// ESEMPIO HANDLER PERSONALIZZATO CON HELPER INLINE
// ===============================
esp_err_t mySimpleAPI(httpd_req_t *req) {
    ESP32WebServer* server = (ESP32WebServer*)req->user_ctx;
    server->currentRequest = req;
    
    String json = server->readJSONBody();     // Helper inline!
    
    if (json.length() == 0) {
        server->sendJSONError("No data");     // Helper inline!
        server->currentRequest = nullptr;
        return ESP_OK;
    }
    
    // La tua logica qui...
    
    server->sendJSONSuccess("Done!");        // Helper inline!
    server->currentRequest = nullptr;
    return ESP_OK;
}

#endif
// Setup nel tuo Arduino:
// webServer.addHandler("/api/my-endpoint", HTTP_POST, mySimpleAPI);