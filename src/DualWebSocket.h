#ifndef DUAL_WEBSOCKET_H
#define DUAL_WEBSOCKET_H

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_psram.h>
extern "C" {
    #include "esp_http_server.h"
    #include "esp_event.h"
    #include <lwip/sockets.h>
    #include <lwip/netdb.h>
    #include <arpa/inet.h>
}

enum WSEventType {
    WS_EVT_CONNECT,
    WS_EVT_DISCONNECT,
    WS_EVT_DATA,
    WS_EVT_ERROR,
    WS_EVT_PONG
};

struct ProxyConfig {
    bool check_proxy_headers;
    String trusted_proxies;
    String base_path;
    
    ProxyConfig() : 
        check_proxy_headers(false),
        trusted_proxies(""),
        base_path("")
    {}
};

class WebSocketServer {
public:
    struct WSClient {
        int id;
        bool inUse;
        char remoteIP[16];
        uint32_t lastSeen;
        uint32_t errorCount;
    };

    typedef std::function<void(WSEventType type, WSClient* client, uint8_t* data, size_t len, void* arg)> WSEventCallback;

    WebSocketServer(uint16_t port, ProxyConfig proxy_config = ProxyConfig()) : 
        _port(port), 
        _server(nullptr),
        _proxy_config(proxy_config) {
        memset(_clients, 0, sizeof(_clients));
        memset(_sendBuffer, 0, sizeof(_sendBuffer));
        _onEvent = nullptr;
        _lastCheck = 0;
        _activeClients = 0;
    }

    ~WebSocketServer() {
        stop();
    }

    void stop() {
        if (_server) {
            httpd_stop(_server);
            _server = nullptr;
        }
        _activeClients = 0;
    }

    bool begin() {
        stop();
        delay(1000);

        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.server_port = _port;
        config.max_open_sockets = 6;
        config.stack_size = 8192;
        config.core_id = tskNO_AFFINITY;
        config.task_priority = 5;
        config.lru_purge_enable = true;
        //config.ctrl_port = _port + 1;
		if (_port == 81) {
			config.ctrl_port = 181;
		} else if (_port == 82) {
			config.ctrl_port = 182;
		}

        Serial.printf("Starting WebSocket server on port %d\n", _port);

        esp_err_t err = httpd_start(&_server, &config);
        if (err != ESP_OK) {
            Serial.printf("httpd_start failed on port %d with error %d\n", _port, err);
            return false;
        }

        String ws_path = _proxy_config.base_path + "/ws";
        httpd_uri_t ws = {
            .uri = ws_path.c_str(),
            .method = HTTP_GET,
            .handler = _wsHandler,
            .user_ctx = this,
            .is_websocket = true
        };

        err = httpd_register_uri_handler(_server, &ws);
        if (err != ESP_OK) {
            Serial.printf("httpd_register_uri_handler failed with error %d\n", err);
            stop();
            return false;
        }

        Serial.printf("WebSocket server started successfully on port %d\n", _port);
        return true;
    }

    void onEvent(WSEventCallback cb) { 
        _onEvent = cb; 
    }

    bool hasClients() {
        checkClients();
        return _activeClients > 0;
    }

    bool isClientValid(int clientId) {
        if (!_server) return false;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (_clients[i].inUse && _clients[i].id == clientId) {
                if (isClientConnected(clientId)) {
                    return _clients[i].errorCount < MAX_ERRORS;
                }
                return false;
            }
        }
        return false;
    }

    bool isClientConnected(int client_id) {
        int ret;
        socklen_t len = sizeof(ret);
        
        if (getsockopt(client_id, SOL_SOCKET, SO_ERROR, &ret, &len) < 0) {
            Serial.printf("Socket check failed for client %d\n", client_id);
            removeClient(client_id);
            return false;
        }
        
        if (ret != 0) {
            Serial.printf("Socket error %d for client %d\n", ret, client_id);
            removeClient(client_id);
            return false;
        }

        return true;
    }

    void checkClients() {
        uint32_t now = millis();
        if (now - _lastCheck >= CHECK_INTERVAL) {
            _activeClients = 0;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (_clients[i].inUse) {
                    if (!isClientConnected(_clients[i].id) || 
                        (now - _clients[i].lastSeen >= CLIENT_TIMEOUT) ||
                        _clients[i].errorCount >= MAX_ERRORS) {
                        Serial.printf("Client %d removed during check (timeout: %d, errors: %d)\n", 
                            _clients[i].id, 
                            (now - _clients[i].lastSeen >= CLIENT_TIMEOUT),
                            _clients[i].errorCount);
                        removeClient(_clients[i].id);
                    } else {
                        _activeClients++;
                    }
                }
            }
            _lastCheck = now;
        }
    }

    bool sendSync(int client_id, const uint8_t* data, size_t len, uint32_t timeout_ms = 5000) {
        if (!_server || len == 0) return false;

        //checkClients();

        if (!isClientValid(client_id)) {
            Serial.printf("Send Sync failed: client %d invalid\n", client_id);
            return false;
        }

        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        
        if (setsockopt(client_id, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
            Serial.printf("Send Sync failed: cannot set timeout for client %d\n", client_id);
            return false;
        }

        uint8_t header[10] = {0};
        size_t header_len = 2;
        header[0] = 0x81;  // FIN + text frame
        
        if (len <= 125) {
            header[1] = len;
        } 
        else if (len <= 65535) {
            header[1] = 126;
            header[2] = (len >> 8) & 0xFF;
            header[3] = len & 0xFF;
            header_len = 4;
        } 
        else {
            header[1] = 127;
            for (int i = 0; i < 8; i++) {
                header[2+i] = (len >> ((7-i)*8)) & 0xFF;
            }
            header_len = 10;
        }

        uint32_t start_time = millis();
        
        ssize_t sent = send(client_id, header, header_len, 0);
        if (sent != header_len) {
            Serial.printf("Send Sync header failed for client %d\n", client_id);
            handleError(client_id, ESP_FAIL, "Send Sync Header");
            return false;
        }

        sent = send(client_id, data, len, 0);
        if (sent != len) {
            Serial.printf("Send Sync data failed for client %d\n", client_id);
            handleError(client_id, ESP_FAIL, "Send Sync Data");
            return false;
        }

        uint32_t elapsed = millis() - start_time;
        if (elapsed >= timeout_ms) {
            Serial.printf("Send Sync timeout after %dms for client %d\n", elapsed, client_id);
            handleError(client_id, ESP_ERR_TIMEOUT, "Send Sync Timeout");
            return false;
        }

        updateClientActivity(client_id);
        return true;
    }

    bool sendAsync(int client_id, const uint8_t* data, size_t len) {
        if (!_server || len == 0) {
            Serial.println("Send Async failed: server null or zero length");
            return false;
        }

        uint8_t* sendBuffer = (uint8_t*)malloc(len);
        if (!sendBuffer) {
            Serial.printf("Send Async failed: cannot allocate %d bytes\n", len);
            return false;
        }

        memcpy(sendBuffer, data, len);

        httpd_ws_frame_t ws_pkt;
        memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
        ws_pkt.payload = sendBuffer;
        ws_pkt.len = len;
        ws_pkt.type = HTTPD_WS_TYPE_TEXT;
        ws_pkt.final = true;

        esp_err_t ret = httpd_ws_send_frame_async(_server, client_id, &ws_pkt);
        if (ret != ESP_OK) {
            free(sendBuffer);
            handleError(client_id, ret, "Send Async");
            return false;
        }

        updateClientActivity(client_id);
        return true;
    }

    bool broadcastSync(const uint8_t* data, size_t len) {
        checkClients();
        if (_activeClients == 0) return false;

        bool success = true;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (_clients[i].inUse && _clients[i].errorCount < MAX_ERRORS) {
                if (!sendSync(_clients[i].id, data, len)) {
                    success = false;
                }
            }
        }
        return success;
    }

    bool broadcastAsync(const uint8_t* data, size_t len) {
        checkClients();
        if (_activeClients == 0) return false;

        bool success = true;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (_clients[i].inUse && _clients[i].errorCount < MAX_ERRORS) {
                if (!sendAsync(_clients[i].id, data, len)) {
                    success = false;
                }
            }
        }
        return success;
    }

    // DIAGNOSTICA
    void printBufferStatus() {
        Serial.printf("Send Buffer: %d bytes available\n", SEND_BUFFER_SIZE);
        Serial.printf("Free heap: %d bytes\n", esp_get_free_heap_size());
    }

    void printBroadcastStats() {
        checkClients();
        Serial.printf("=== Broadcast Status ===\n");
        Serial.printf("Active clients: %d/%d\n", _activeClients, MAX_CLIENTS);
        Serial.printf("Buffer size: %d bytes\n", SEND_BUFFER_SIZE);
        
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (_clients[i].inUse) {
                Serial.printf("Client %d: %s (errors: %d)\n", 
                    _clients[i].id, 
                    (_clients[i].errorCount < MAX_ERRORS) ? "ACTIVE" : "DISABLED",
                    _clients[i].errorCount);
            }
        }
        Serial.println("=======================");
    }

    size_t getMaxBufferSize() const {
        return SEND_BUFFER_SIZE;
    }

private:
    static const int MAX_CLIENTS = 4;
    static const uint32_t CHECK_INTERVAL = 5000;  // 5 secondi
    static const uint32_t CLIENT_TIMEOUT = 30000; // 30 secondi
    static const int MAX_ERRORS = 3;              // Max errori prima di disconnettere
    static const size_t SEND_BUFFER_SIZE = 4096;  // Buffer fisso condiviso
    
    httpd_handle_t _server;
    uint16_t _port;
    WSClient _clients[MAX_CLIENTS];
    WSEventCallback _onEvent;
    uint32_t _lastCheck;
    int _activeClients;
    ProxyConfig _proxy_config;
    uint8_t _sendBuffer[SEND_BUFFER_SIZE];  // Buffer fisso per operazioni zero-copy

    bool verify_proxy_request(httpd_req_t *req) {
        if (!_proxy_config.check_proxy_headers) return true;

        size_t proto_len = httpd_req_get_hdr_value_len(req, "X-Forwarded-Proto");
        if (proto_len > 0) {
            char* proto = (char*)malloc(proto_len + 1);
            if (httpd_req_get_hdr_value_str(req, "X-Forwarded-Proto", proto, proto_len + 1) == ESP_OK) {
                if (strcmp(proto, "https") != 0) {
                    free(proto);
                    return false;
                }
            }
            free(proto);
        }

        if (!_proxy_config.trusted_proxies.isEmpty()) {
            size_t ip_len = httpd_req_get_hdr_value_len(req, "X-Real-IP");
            if (ip_len > 0) {
                char* ip = (char*)malloc(ip_len + 1);
                if (httpd_req_get_hdr_value_str(req, "X-Real-IP", ip, ip_len + 1) == ESP_OK) {
                    bool trusted = _proxy_config.trusted_proxies.indexOf(ip) >= 0;
                    free(ip);
                    return trusted;
                }
                free(ip);
            }
        }

        return true;
    }

    void handleError(int client_id, esp_err_t error, const char* operation) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (_clients[i].inUse && _clients[i].id == client_id) {
                _clients[i].errorCount++;
                Serial.printf("%s failed: error %d on client %d (errors: %d/%d)\n", 
                    operation, error, client_id, _clients[i].errorCount, MAX_ERRORS);
                
                if (_clients[i].errorCount >= MAX_ERRORS || 
                    error == ESP_ERR_HTTPD_INVALID_REQ) {
                    removeClient(client_id);
                }
                break;
            }
        }
    }

    void updateClientActivity(int client_id) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (_clients[i].inUse && _clients[i].id == client_id) {
                _clients[i].lastSeen = millis();
                break;
            }
        }
    }

    void addClient(int id) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!_clients[i].inUse) {
                _clients[i].inUse = true;
                _clients[i].id = id;
                _clients[i].lastSeen = millis();
                _clients[i].errorCount = 0;
                
                struct sockaddr_in6 addr;
                socklen_t addr_size = sizeof(addr);
                if (getpeername(id, (struct sockaddr *)&addr, &addr_size) == 0) {
                    inet_ntop(AF_INET6, &addr.sin6_addr, _clients[i].remoteIP, sizeof(_clients[i].remoteIP));
                    Serial.printf("Client %d added with id %d from %s\n", i, id, _clients[i].remoteIP);
                }
                _activeClients++;
                break;
            }
        }
    }

    void removeClient(int id) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (_clients[i].inUse && _clients[i].id == id) {
                _clients[i].inUse = false;
                Serial.printf("Client %d removed\n", i);
                if (_onEvent) {
                    WSClient client = _clients[i];
                    _onEvent(WS_EVT_DISCONNECT, &client, nullptr, 0, nullptr);
                }
                _activeClients--;
                break;
            }
        }
    }

    static esp_err_t _wsHandler(httpd_req_t *req) {
        WebSocketServer* ws = (WebSocketServer*)req->user_ctx;
        if (!ws) return ESP_FAIL;

        if (!ws->verify_proxy_request(req)) {
            return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, 
                "Invalid proxy configuration");
        }

        WSClient client;
        client.id = httpd_req_to_sockfd(req);

        if (req->method == HTTP_GET) {
            ws->addClient(client.id);
            if (ws->_onEvent) {
                ws->_onEvent(WS_EVT_CONNECT, &client, nullptr, 0, nullptr);
            }
            return ESP_OK;
        }

        httpd_ws_frame_t ws_pkt;
        memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
        
        esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
        if (ret != ESP_OK) {
            if (ws->_onEvent) {
                uint16_t error = ret;
                ws->_onEvent(WS_EVT_ERROR, &client, nullptr, 0, &error);
            }
            return ret;
        }

        uint8_t *buf = (uint8_t *)malloc(ws_pkt.len + 1);
        if (!buf) return ESP_ERR_NO_MEM;

        ws_pkt.payload = buf;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        
        if (ret == ESP_OK) {
            ws->updateClientActivity(client.id);
            
            if (ws->_onEvent) {
                if (ws_pkt.type == HTTPD_WS_TYPE_PONG) {
                    ws->_onEvent(WS_EVT_PONG, &client, nullptr, 0, nullptr);
                } else if (ws_pkt.type == HTTPD_WS_TYPE_CLOSE) {
                    ws->_onEvent(WS_EVT_DISCONNECT, &client, nullptr, 0, nullptr);
                } else if (ws_pkt.type == HTTPD_WS_TYPE_BINARY) {
                    ws->_onEvent(WS_EVT_DATA, &client, (uint8_t*)ws_pkt.payload, ws_pkt.len, nullptr);
                } else if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) {
                    ws->_onEvent(WS_EVT_DATA, &client, (unsigned char*)ws_pkt.payload, ws_pkt.len, nullptr);
                }
            }
        }

        free(buf);
        return ret;
    }
};

class DualWebSocket {
public:
    DualWebSocket() : 
        dataServer(81),    
        controlServer(82)  
    {}

    bool begin() {
        stop();
        delay(1000);

        Serial.println("Starting data server...");
        if (!dataServer.begin()) {
            Serial.println("Failed to start data server");
            return false;
        }
        
        delay(2000);

        Serial.println("Starting control server...");
        if (!controlServer.begin()) {
            Serial.println("Failed to start control server");
            dataServer.stop();
            delay(1000);
            return false;
        }

        Serial.println("Both servers started successfully");
        return true;
    }

    void stop() {
        controlServer.stop();
        delay(1000);
        dataServer.stop();
        delay(1000);
    }

    void onDataEvent(WebSocketServer::WSEventCallback cb) {
        dataServer.onEvent(cb);
    }

    void onControlEvent(WebSocketServer::WSEventCallback cb) {
        controlServer.onEvent(cb);
    }

    bool hasDataClients() {
        return dataServer.hasClients();
    }

    bool hasControlClients() {
        return controlServer.hasClients();
    }

    // ===== METODI TRADIZIONALI =====
    bool sendDataToClient(int clientId, const uint8_t* data, size_t len, bool async = false) {
        if (async) {
            return dataServer.sendAsync(clientId, data, len);
        } else {
            return dataServer.sendSync(clientId, data, len);
        }
    }

    bool sendControlToClient(int clientId, const uint8_t* data, size_t len, bool async = false) {
        if (async) {
            return controlServer.sendAsync(clientId, data, len);
        } else {
            return controlServer.sendSync(clientId, data, len);
        }
    }

    bool sendDataSync(const uint8_t* data, size_t len) {
        return dataServer.broadcastSync(data, len);
    }

    bool sendControlSync(const uint8_t* data, size_t len) {
        return controlServer.broadcastSync(data, len);
    }

    bool sendDataAsync(const uint8_t* data, size_t len) {
        return dataServer.broadcastAsync(data, len);
    }

    bool sendControlAsync(const uint8_t* data, size_t len) {
        return controlServer.broadcastAsync(data, len);
    }

    // Overload per stringhe
    bool sendDataToClient(int clientId, const char* data, size_t len, bool async = false) {
        return sendDataToClient(clientId, (const uint8_t*)data, len, async);
    }

    bool sendControlToClient(int clientId, const char* data, size_t len, bool async = false) {
        return sendControlToClient(clientId, (const uint8_t*)data, len, async);
    }

    bool sendDataSync(const char* data, size_t len) {
        return sendDataSync((const uint8_t*)data, len);
    }

    bool sendControlSync(const char* data, size_t len) {
        return sendControlSync((const uint8_t*)data, len);
    }

    bool sendDataAsync(const char* data, size_t len) {
        return sendDataAsync((const uint8_t*)data, len);
    }

    bool sendControlAsync(const char* data, size_t len) {
        return sendControlAsync((const uint8_t*)data, len);
    }

    // ===== DIAGNOSTICA =====
    void checkClients() {
        dataServer.checkClients();
        controlServer.checkClients();
    }

    void printStatus() {
        Serial.println("=== DUAL WEBSOCKET STATUS ===");
        Serial.printf("Data Server (Port 81): %s, Clients: %d\n", 
            hasDataClients() ? "ACTIVE" : "IDLE", 
            hasDataClients() ? 1 : 0);
        Serial.printf("Control Server (Port 82): %s, Clients: %d\n", 
            hasControlClients() ? "ACTIVE" : "IDLE", 
            hasControlClients() ? 1 : 0);
        
        dataServer.printBroadcastStats();
        controlServer.printBroadcastStats();
        
        Serial.printf("Free heap: %d bytes\n", esp_get_free_heap_size());
        Serial.println("============================");
    }

    size_t getMaxBufferSize() const {
        return dataServer.getMaxBufferSize();
    }

private:
    WebSocketServer dataServer;
    WebSocketServer controlServer;
};

#endif // DUAL_WEBSOCKET_H

/*
ESEMPI D'USO OTTIMIZZATI:

// 1. BROADCAST DA CODA (ZERO-COPY)
QueueHandle_t dataQueue = xQueueCreate(10, 1024);
dualWS.sendDataFromQueue(dataQueue, 1024);

// 2. INVIO A CLIENT SPECIFICO DA STREAM BUFFER
StreamBufferHandle_t streamBuffer = xStreamBufferCreate(4096, 1);
dualWS.sendDataToClientFromStreamBuffer(clientId, streamBuffer);

// 3. BROADCAST DA MESSAGE BUFFER (DIMENSIONI VARIABILI)
MessageBufferHandle_t msgBuffer = xMessageBufferCreate(4096);
dualWS.sendControlFromMessageBuffer(msgBuffer);

// 4. DIAGNOSTICA
dualWS.printStatus();

VANTAGGI:
✅ Zero malloc/free - Buffer fisso da 4KB
✅ Zero memcpy - Receive diretto nel buffer WebSocket
✅ Zero mutex - Solo operazioni sincronizzate
✅ Thread-safe - Code RTOS già protette
✅ Determinismo - Tempi prevedibili
✅ Efficienza - Massima velocità per ESP32
*/
