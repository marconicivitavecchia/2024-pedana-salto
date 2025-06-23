#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include <map>
#include <string>
#include <vector>

class ESP32WebServer {
public:
	httpd_req_t* currentRequest;
    struct Config {
        uint16_t port;
        bool check_proxy_headers;
        String trusted_proxies;
        String base_path;
        
        Config() : 
            port(80),
            check_proxy_headers(false),
            trusted_proxies(""),
            base_path("") 
        {}
    };

    // Struttura avanzata per le pagine personalizzate
    struct CustomPage {
		String uri;
		String title;
		String content;
		String contentType;
		int statusCode;
		std::map<String, String> headers;
		
		// Costruttore di default
		CustomPage() : 
			uri(""), 
			content(""), 
			contentType("text/html"), 
			statusCode(200), 
			title("") {}
		
		// Costruttore con parametri (quello esistente)
		CustomPage(const String& u, const String& c, const String& ct = "text/html", int sc = 200, const String& t = "") :
			uri(u), content(c), contentType(ct), statusCode(sc), title(t) {}
			
		void addHeader(const String& name, const String& value) {
			headers[name] = value;
		}
	};

    // Struttura per informazioni proxy
    struct ProxyInfo {
        String realIP;
        String forwardedFor;
        String originalHost;
        String protocol;
        bool isProxied;
        
        ProxyInfo() : isProxied(false) {}
    };

    ESP32WebServer() : server(nullptr), currentRequest(nullptr) {
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
        
        //setupDefaultPages();
		//registerHandlers();
        
        ESP_LOGI(TAG, "HTTP Server Started on port %d", config.port);
        return true;
    }
    
    void stop() {
        if (server) {
            httpd_stop(server);
            server = nullptr;
        }
        customHandlers.clear();
        customPages.clear();
    }
	
	 void enableFileHandler() {
        registerHandlers();
    }
	
	/*
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
	*/
	bool addHandler(const char* uri, httpd_method_t method, 
				   esp_err_t (*handler)(httpd_req_t *r),
				   void* ctx = nullptr) {
		
		Serial.print("Attempting to register handler for URI: ");
		Serial.println(uri);
		
		if (server == nullptr) {
			Serial.println("ERROR: Server handle is NULL!");
			return false;
		}
		
		httpd_uri_t h = {
			.uri = uri,
			.method = method,
			.handler = handler,
			.user_ctx = ctx ? ctx : this
			//.user_ctx = NULL
		};
		
		esp_err_t ret = httpd_register_uri_handler(server, &h);
		Serial.print("Registration result: ");
		Serial.println(esp_err_to_name(ret));
		
		return ret == ESP_OK;
	}
	
	// *** HELPER INLINE PER API REST ***
	
	// Invia risposta JSON con status code
    inline void sendJSON(const String& json, int statusCode = 200) {
        if (currentRequest) {
            addAPIHeaders();
            httpd_resp_set_status(currentRequest, getStatusString(statusCode));
            httpd_resp_send(currentRequest, json.c_str(), json.length());
        }
    }
	
	// Aggiunge header standard per API REST
    inline void addAPIHeaders(httpd_req_t *req) {
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
        httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
        httpd_resp_set_type(req, "application/json");
    }
	
	// Legge corpo JSON da richiesta POST
    inline String readJSONBody(httpd_req_t *req) {
        if (req->content_len == 0) return "";
        
        char* buffer = (char*)malloc(req->content_len + 1);
        if (!buffer) return "";
        
        int totalReceived = 0;
        int remaining = req->content_len;
        
        while (remaining > 0) {
            int received = httpd_req_recv(req, buffer + totalReceived, remaining);
            if (received <= 0) {
                if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                    continue; // Retry su timeout
                }
                free(buffer);
                return "";
            }
            totalReceived += received;
            remaining -= received;
        }
        
        buffer[totalReceived] = '\0';
        String result = String(buffer);
        free(buffer);
        return result;
    }
	
	// Legge corpo JSON usando currentRequest
    inline String readJSONBody() {
        return currentRequest ? readJSONBody(currentRequest) : "";
    }
	
	// Invia errore JSON standard
    inline void sendJSONError(const String& error, int statusCode = 400) {
        String json = "{\"success\":false,\"error\":\"" + error + "\"}";
        sendJSON(json, statusCode);
    }
	
	// Invia successo JSON standard
    inline void sendJSONSuccess(const String& message = "OK", int statusCode = 200) {
        String json = "{\"success\":true,\"message\":\"" + message + "\"}";
        sendJSON(json, statusCode);
    }

    // *** API SEMPLICE (Approccio 2) ***
    
    // Metodo base per route generiche
    void route(const String& path, const String& response, const String& contentType = "text/html") {
        CustomPage page(path, response, contentType);
        customPages[path] = page;
        ESP_LOGI(TAG, "Added route: %s (%s)", path.c_str(), contentType.c_str());
    }
    
    // Pagine HTML veloci con stile automatico
    void page(const String& path, const String& title, const String& body) {
        String html = buildStyledHtml(title, body);
        CustomPage page(path, html, "text/html", 200, title);
        customPages[path] = page;
        ESP_LOGI(TAG, "Added styled page: %s", path.c_str());
    }
    
    // API JSON immediate
    void api(const String& path, const String& jsonResponse) {
        CustomPage page(path, jsonResponse, "application/json");
        page.addHeader("Access-Control-Allow-Origin", "*");  // CORS automatico per API
        customPages[path] = page;
        ESP_LOGI(TAG, "Added API endpoint: %s", path.c_str());
    }
	
	// Aggiunge header API usando currentRequest
    inline void addAPIHeaders() {
        if (currentRequest) {
            addAPIHeaders(currentRequest);
        }
    }
    
    
    // *** API AVANZATA (Approccio 1) ***
    
    // Aggiunge una pagina con controllo completo
    void addPage(const String& uri, const String& content, const String& contentType = "text/html", int statusCode = 200, const String& title = "") {
        CustomPage page(uri, content, contentType, statusCode, title);
        customPages[uri] = page;
        ESP_LOGI(TAG, "Added custom page: %s", uri.c_str());
    }
    
    // Aggiunge una pagina HTML con template avanzato
    void addHtmlPage(const String& uri, const String& title, const String& body) {
        String html = buildAdvancedHtml(title, body);
        CustomPage page(uri, html, "text/html", 200, title);
        customPages[uri] = page;
        ESP_LOGI(TAG, "Added HTML page: %s", uri.c_str());
    }
    
    // Carica una pagina da file
    bool addPageFromFile(const String& uri, const String& filepath, const String& contentType = "") {
        return loadFile(uri, filepath);  // Riusa il metodo semplice
    }
    
    // Carica file come route (semplice)
    bool loadFile(const String& path, const String& filePath) {
        if (!LittleFS.exists(filePath)) {
            ESP_LOGW(TAG, "File not found: %s", filePath.c_str());
            return false;
        }
        
        File file = LittleFS.open(filePath, "r");
        if (!file) return false;
        
        String content = file.readString();
        file.close();
        
        String contentType = detectMimeType(filePath);
        CustomPage page(path, content, contentType);
        customPages[path] = page;
        
        ESP_LOGI(TAG, "Loaded file: %s -> %s (%s)", filePath.c_str(), path.c_str(), contentType.c_str());
        return true;
    }
    
    // Genera automaticamente una pagina di indice
    void generateIndexPage() {
        String indexContent = buildIndexContent();
        addHtmlPage("/", "ESP32 Web Server", indexContent);
    }
    
    // Aggiunge header personalizzato a una pagina esistente
    bool addPageHeader(const String& uri, const String& headerName, const String& headerValue) {
        auto it = customPages.find(uri);
        if (it != customPages.end()) {
            it->second.addHeader(headerName, headerValue);
            return true;
        }
        return false;
    }

    httpd_handle_t getServer() { return server; }
    
    // Compatibilità con WebServer standard
    void send(int code, const char* content_type, const String& content) {
        if (currentRequest) {
            httpd_resp_set_status(currentRequest, getStatusString(code));
            httpd_resp_set_type(currentRequest, content_type);
            httpd_resp_send(currentRequest, content.c_str(), content.length());
        }
    }

    void on(const char* uri, std::function<void()> handler) {
        customHandlers[String(uri)] = handler;
    }

    void handleClient() {
        // Non necessario per ESP-IDF
    }

    // *** METODI PROXY (Nuovi) ***
    
    String getClientIP() {
        if (currentRequest) {
            ProxyInfo proxy = extractProxyInfo(currentRequest);
            if (!proxy.realIP.isEmpty()) return proxy.realIP;
            if (!proxy.forwardedFor.isEmpty()) {
                int commaIndex = proxy.forwardedFor.indexOf(',');
                String firstIP = (commaIndex > 0) ? proxy.forwardedFor.substring(0, commaIndex) : proxy.forwardedFor;
                firstIP.trim();
                return firstIP;
            }
        }
        return "unknown";
    }
    
    String getOriginalHost() {
        if (currentRequest) {
            ProxyInfo proxy = extractProxyInfo(currentRequest);
            return proxy.originalHost.isEmpty() ? "localhost" : proxy.originalHost;
        }
        return "localhost";
    }
    
    String getProtocol() {
        if (currentRequest) {
            ProxyInfo proxy = extractProxyInfo(currentRequest);
            return proxy.protocol.isEmpty() ? "http" : proxy.protocol;
        }
        return "http";
    }
    
    bool isProxiedRequest() {
        if (currentRequest) {
            ProxyInfo proxy = extractProxyInfo(currentRequest);
            return proxy.isProxied;
        }
        return false;
    }
    
    String getFullUrl() {
        if (currentRequest) {
            return getProtocol() + "://" + getOriginalHost() + String(currentRequest->uri);
        }
        return "";
    }

    // Metodi di utilità e debug
    size_t getCustomHandlerCount() { return customHandlers.size(); }
    size_t getCustomPageCount() { return customPages.size(); }

    void info() {
        Serial.println("=== ESP32 Web Server - Unified ===");
        Serial.printf("Port: %d\n", config.port);
        Serial.printf("Proxy Headers: %s\n", config.check_proxy_headers ? "Enabled" : "Disabled");
        if (!config.trusted_proxies.isEmpty()) {
            Serial.printf("Trusted Proxies: %s\n", config.trusted_proxies.c_str());
        }
        Serial.printf("Custom Pages: %d\n", customPages.size());
        for (auto& pair : customPages) {
            Serial.printf("  %s -> %s (%s)\n", 
                pair.first.c_str(), 
                pair.second.title.isEmpty() ? "No title" : pair.second.title.c_str(),
                pair.second.contentType.c_str());
        }
        Serial.printf("Dynamic Handlers: %d\n", customHandlers.size());
        for (auto& pair : customHandlers) {
            Serial.printf("  %s (dynamic)\n", pair.first.c_str());
        }
        Serial.println("==================================");
    }

    void listAll() { info(); }  // Alias per compatibilità
	

private:
    std::map<String, std::function<void()>> customHandlers;
    std::map<String, CustomPage> customPages;

    static const char* TAG;
    httpd_handle_t server;
    std::map<std::string, std::string> mime_types;
    Config config;

    void setupDefaultPages() {
        if (customPages.find("/") == customPages.end()) {
            generateIndexPage();
        }
    }

    void registerHandlers() {
        httpd_uri_t file_handler = {
            .uri = "/*",
            .method = HTTP_GET,
            .handler = handle_request,// chiamato solo con il wildcard
            .user_ctx = this
        };
        httpd_register_uri_handler(server, &file_handler);
    }

    // Helper per build HTML
    String buildStyledHtml(const String& title, const String& body) {
        String html = "<!DOCTYPE html><html><head><title>" + title + "</title>";
        html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
        html += "<style>*{margin:0;padding:0;box-sizing:border-box}";
        html += "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;line-height:1.6;color:#333;background:#f8f9fa}";
        html += ".container{max-width:1000px;margin:0 auto;padding:20px}";
        html += "h1{color:#2c3e50;margin-bottom:20px;padding-bottom:10px;border-bottom:3px solid #3498db}";
        html += "nav{background:#34495e;padding:10px 0;margin:-20px -20px 20px -20px}";
        html += "nav a{color:white;text-decoration:none;margin:0 15px;padding:5px 10px;border-radius:4px}";
        html += "nav a:hover{background:#2c3e50}</style></head><body>";
        html += buildNavigation();
        html += "<div class='container'><h1>" + title + "</h1>" + body + "</div></body></html>";
        return html;
    }
    
    String buildAdvancedHtml(const String& title, const String& body) {
        String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>" + title + "</title>";
        html += "<style>body{font-family:Arial,sans-serif;margin:40px;background:#f5f5f5}";
        html += ".container{max-width:800px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}";
        html += "h1{color:#333;border-bottom:2px solid #007bff;padding-bottom:10px}";
        html += "a{color:#007bff;text-decoration:none}a:hover{text-decoration:underline}</style></head>";
        html += "<body><div class='container'><h1>" + title + "</h1>" + body;
        html += "<hr><p><a href='/'>← Torna alla home</a></p></div></body></html>";
        return html;
    }
    
    String buildNavigation() {
        String nav = "<nav><div class='container'><a href='/'>Home</a>";
        for (auto& page : customPages) {
            if (page.first != "/" && page.first.indexOf("/api/") != 0) {
                String title = page.second.title.isEmpty() ? page.first : page.second.title;
                nav += "<a href='" + page.first + "'>" + title + "</a>";
            }
        }
        nav += "</div></nav>";
        return nav;
    }
    
    String buildIndexContent() {
        String content = "<h2>📄 Pagine Disponibili</h2><ul>";
        for (auto& pair : customPages) {
            if (pair.first != "/") {
                String title = pair.second.title.isEmpty() ? pair.first : pair.second.title;
                content += "<li><a href='" + pair.first + "'>" + title + "</a>";
                if (pair.first.indexOf("/api/") == 0) content += " <em>(API)</em>";
                content += "</li>";
            }
        }
        content += "</ul>";
        
        if (!customHandlers.empty()) {
            content += "<h2>⚡ Handler Dinamici</h2><ul>";
            for (auto& pair : customHandlers) {
                content += "<li><a href='" + pair.first + "'>" + pair.first + " (Dynamic)</a></li>";
            }
            content += "</ul>";
        }
        
        content += "<h2>🔧 Server Info</h2>";
        content += "<p><strong>Porta:</strong> " + String(config.port) + "</p>";
        content += "<p><strong>Proxy Headers:</strong> " + String(config.check_proxy_headers ? "Abilitati" : "Disabilitati") + "</p>";
        content += "<p><strong>Pagine caricate:</strong> " + String(customPages.size()) + "</p>";
        content += "<p><strong>Handler dinamici:</strong> " + String(customHandlers.size()) + "</p>";
        
        return content;
    }
    
    String detectMimeType(const String& filepath) {
        const char* ext = strrchr(filepath.c_str(), '.');
        if (ext) {
            auto mime_it = mime_types.find(ext);
            if (mime_it != mime_types.end()) {
                return mime_it->second.c_str();
            }
        }
        return "text/plain";
    }
    
    const char* getStatusString(int code) {
        switch(code) {
            case 200: return "200 OK";
            case 404: return "404 Not Found";
            case 500: return "500 Internal Server Error";
            case 403: return "403 Forbidden";
            default: return "200 OK";
        }
    }

    // Gestione proxy completa
    static ProxyInfo extractProxyInfo(httpd_req_t *req) {
        ProxyInfo info;
        
        // X-Forwarded-Proto
        size_t proto_len = httpd_req_get_hdr_value_len(req, "X-Forwarded-Proto");
        if (proto_len > 0) {
            char* proto = (char*)malloc(proto_len + 1);
            if (proto && httpd_req_get_hdr_value_str(req, "X-Forwarded-Proto", proto, proto_len + 1) == ESP_OK) {
                info.protocol = String(proto);
                info.isProxied = true;
            }
            if (proto) free(proto);
        }
        
        // X-Real-IP
        size_t real_ip_len = httpd_req_get_hdr_value_len(req, "X-Real-IP");
        if (real_ip_len > 0) {
            char* real_ip = (char*)malloc(real_ip_len + 1);
            if (real_ip && httpd_req_get_hdr_value_str(req, "X-Real-IP", real_ip, real_ip_len + 1) == ESP_OK) {
                info.realIP = String(real_ip);
                info.isProxied = true;
            }
            if (real_ip) free(real_ip);
        }
        
        // X-Forwarded-For
        size_t fwd_for_len = httpd_req_get_hdr_value_len(req, "X-Forwarded-For");
        if (fwd_for_len > 0) {
            char* fwd_for = (char*)malloc(fwd_for_len + 1);
            if (fwd_for && httpd_req_get_hdr_value_str(req, "X-Forwarded-For", fwd_for, fwd_for_len + 1) == ESP_OK) {
                info.forwardedFor = String(fwd_for);
                info.isProxied = true;
            }
            if (fwd_for) free(fwd_for);
        }
        
        // X-Forwarded-Host
        size_t fwd_host_len = httpd_req_get_hdr_value_len(req, "X-Forwarded-Host");
        if (fwd_host_len > 0) {
            char* fwd_host = (char*)malloc(fwd_host_len + 1);
            if (fwd_host && httpd_req_get_hdr_value_str(req, "X-Forwarded-Host", fwd_host, fwd_host_len + 1) == ESP_OK) {
                info.originalHost = String(fwd_host);
                info.isProxied = true;
            }
            if (fwd_host) free(fwd_host);
        }
        
        return info;
    }

    static bool verify_proxy_request(httpd_req_t *req, const Config& config) {
        if (!config.check_proxy_headers) return true;

        ProxyInfo proxy = extractProxyInfo(req);
        
        if (!proxy.isProxied) {
            ESP_LOGW("ESP32_WEB_SERVER", "Expected proxy headers but none found");
            return false;
        }

        // Verifica protocollo HTTPS
        if (!proxy.protocol.isEmpty() && proxy.protocol != "https") {
            ESP_LOGW("ESP32_WEB_SERVER", "Non-HTTPS request via proxy: %s", proxy.protocol.c_str());
            return false;
        }

        // Verifica IP trusted
        if (!config.trusted_proxies.isEmpty()) {
            String clientIP = proxy.realIP;
            if (clientIP.isEmpty() && !proxy.forwardedFor.isEmpty()) {
                int commaIndex = proxy.forwardedFor.indexOf(',');
                clientIP = (commaIndex > 0) ? proxy.forwardedFor.substring(0, commaIndex) : proxy.forwardedFor;
                clientIP.trim();
            }
            
            if (!clientIP.isEmpty()) {
                bool trusted = config.trusted_proxies.indexOf(clientIP) >= 0;
                if (!trusted) {
                    ESP_LOGW("ESP32_WEB_SERVER", "Untrusted client IP: %s", clientIP.c_str());
                    return false;
                }
            }
        }

        ESP_LOGI("ESP32_WEB_SERVER", "Proxy request - Proto: %s, Host: %s, ClientIP: %s", 
                proxy.protocol.c_str(), 
                proxy.originalHost.c_str(), 
                proxy.realIP.isEmpty() ? proxy.forwardedFor.c_str() : proxy.realIP.c_str());

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

        server->currentRequest = req;
        String requestUri = String(req->uri);
        
        // PRIORITÀ 1: PAGINE PERSONALIZZATE
        auto page_it = server->customPages.find(requestUri);
        if (page_it != server->customPages.end()) {
            const CustomPage& page = page_it->second;
            
            httpd_resp_set_status(req, server->getStatusString(page.statusCode));
            httpd_resp_set_type(req, page.contentType.c_str());
            
            // Headers personalizzati
            for (auto& header : page.headers) {
                httpd_resp_set_hdr(req, header.first.c_str(), header.second.c_str());
            }
            
            // Cache headers
            if (server->config.check_proxy_headers) {
                httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=3600");
            } else {
                httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=86400");
            }
            
            httpd_resp_send(req, page.content.c_str(), page.content.length());
            server->currentRequest = nullptr;
            return ESP_OK;
        }
        
        // PRIORITÀ 2: HANDLER DINAMICI
        auto handler_it = server->customHandlers.find(requestUri);
        if (handler_it != server->customHandlers.end()) {
            handler_it->second();
            server->currentRequest = nullptr;
            return ESP_OK;
        }
        
        // PRIORITÀ 3: FILE DA LITTLEFS
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
                server->currentRequest = nullptr;
                return httpd_resp_send_500(req);
            }

            const char* ext = strrchr(filepath, '.');
            if (ext) {
                auto mime_it = server->mime_types.find(ext);
                if (mime_it != server->mime_types.end()) {
                    httpd_resp_set_type(req, mime_it->second.c_str());
                }
            }

            httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=86400");

            size_t filesize = file.size();
            char size_str[32];
            snprintf(size_str, sizeof(size_str), "%u", filesize);
            httpd_resp_set_hdr(req, "Content-Length", size_str);

            char chunk[1024];
            size_t read;
            while ((read = file.read((uint8_t*)chunk, sizeof(chunk))) > 0) {
                if (httpd_resp_send_chunk(req, chunk, read) != ESP_OK) {
                    file.close();
                    server->currentRequest = nullptr;
                    return ESP_FAIL;
                }
            }
            file.close();
            httpd_resp_send_chunk(req, NULL, 0);
            server->currentRequest = nullptr;
            return ESP_OK;
        }

        server->currentRequest = nullptr;
        return httpd_resp_send_404(req);
    }
};

const char* ESP32WebServer::TAG = "ESP32_WEB_SERVER";