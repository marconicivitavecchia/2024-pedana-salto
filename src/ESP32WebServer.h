#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include <map>
#include <string>

class ESP32WebServer {
public:
    struct Config {
        uint16_t port;
        bool check_proxy_headers;
        String trusted_proxies;
        String base_path;
        
        // Costruttore con valori di default
        Config() : 
            port(80),
            check_proxy_headers(false),
            trusted_proxies(""),
            base_path("") 
        {}
    };

    ESP32WebServer() : server(nullptr) {
        mime_types = {
            {".html", "text/html"},
            {".css", "text/css"},
            {".js", "application/javascript"},
            {".json", "application/json"},
            {".png", "image/png"},
            {".jpg", "image/jpeg"},
            {".ico", "image/x-icon"},
            {".svg", "image/svg+xml"},
            {".woff", "font/woff"},
            {".woff2", "font/woff2"},
            {".ttf", "font/ttf"},
            {".eot", "application/vnd.ms-fontobject"},
            {".mp3", "audio/mpeg"},
            {".wav", "audio/wav"},
            {".pdf", "application/pdf"},
            {".zip", "application/zip"},
            {".gz", "application/gzip"}
        };
    }
    
    ~ESP32WebServer() {
        stop();
    }
    
    bool begin(const Config& config = Config()) {
        this->config = config;
        
        httpd_config_t http_config = HTTPD_DEFAULT_CONFIG();
        http_config.server_port = config.port;
        http_config.max_uri_handlers = 16;
        http_config.max_open_sockets = 4;
        http_config.uri_match_fn = httpd_uri_match_wildcard;
        http_config.stack_size = 8192;
        http_config.lru_purge_enable = true;
        
        esp_err_t ret = httpd_start(&server, &http_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error starting server!");
            return false;
        }
        
        registerHandlers();
        
        ESP_LOGI(TAG, "HTTP Server Started on port %d", config.port);
        return true;
    }
    
    void stop() {
        if (server) {
            httpd_stop(server);
            server = nullptr;
        }
    }

    bool addHandler(const char* uri, httpd_method_t method, 
                   esp_err_t (*handler)(httpd_req_t *r),
                   void* ctx = nullptr) {
        httpd_uri_t h = {
            .uri = uri,
            .method = method,
            .handler = handler,
            .user_ctx = ctx ? ctx : this
        };
        return httpd_register_uri_handler(server, &h) == ESP_OK;
    }

    httpd_handle_t getServer() { return server; }

private:
    static const char* TAG;
    httpd_handle_t server;
    std::map<std::string, std::string> mime_types;
    Config config;

    void registerHandlers() {
        httpd_uri_t file_handler = {
            .uri = "/*",
            .method = HTTP_GET,
            .handler = handle_request,
            .user_ctx = this
        };
        httpd_register_uri_handler(server, &file_handler);
    }

    static bool verify_proxy_request(httpd_req_t *req, const Config& config) {
        if (!config.check_proxy_headers) return true;

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

        if (!config.trusted_proxies.isEmpty()) {
            size_t ip_len = httpd_req_get_hdr_value_len(req, "X-Real-IP");
            if (ip_len > 0) {
                char* ip = (char*)malloc(ip_len + 1);
                if (httpd_req_get_hdr_value_str(req, "X-Real-IP", ip, ip_len + 1) == ESP_OK) {
                    bool trusted = config.trusted_proxies.indexOf(ip) >= 0;
                    free(ip);
                    return trusted;
                }
                free(ip);
            }
        }

        return true;
    }

    static esp_err_t handle_request(httpd_req_t *req) {
        ESP32WebServer* server = (ESP32WebServer*)req->user_ctx;
        
        if (server->config.check_proxy_headers && !verify_proxy_request(req, server->config)) {
            return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "Invalid proxy configuration");
        }

        if (strstr(req->uri, "/ws") == req->uri) {
            return ESP_OK;
        }

        char filepath[512];
        if (server->config.base_path.length() > 0) {
            snprintf(filepath, sizeof(filepath), "%s%s", 
                    server->config.base_path.c_str(), req->uri);
        } else {
            strlcpy(filepath, req->uri, sizeof(filepath));
        }

        if (strcmp(filepath, "/") == 0 || 
            strcmp(filepath, server->config.base_path.c_str()) == 0) {
            strlcpy(filepath, "/index.html", sizeof(filepath));
        }

        if (LittleFS.exists(filepath)) {
            File file = LittleFS.open(filepath, "r");
            if (!file) {
                return httpd_resp_send_500(req);
            }

            // MIME type
            const char* ext = strrchr(filepath, '.');
            if (ext) {
                auto it = server->mime_types.find(ext);
                if (it != server->mime_types.end()) {
                    httpd_resp_set_type(req, it->second.c_str());
                }
            }

            // Cache Control
            if (server->config.check_proxy_headers) {
                httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=3600");
            } else {
                httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=86400");
            }

            // Content Length
            size_t filesize = file.size();
            char size_str[32];
            snprintf(size_str, sizeof(size_str), "%u", filesize);
            httpd_resp_set_hdr(req, "Content-Length", size_str);

            // Invia il file
            char chunk[1024];
            size_t read;
            while ((read = file.read((uint8_t*)chunk, sizeof(chunk))) > 0) {
                if (httpd_resp_send_chunk(req, chunk, read) != ESP_OK) {
                    file.close();
                    return ESP_FAIL;
                }
            }
            file.close();
            httpd_resp_send_chunk(req, NULL, 0);
            return ESP_OK;
        }

        return httpd_resp_send_404(req);
    }
};

const char* ESP32WebServer::TAG = "ESP32_WEB_SERVER";