#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include "DualWebSocket.h"
#include "ADS1256_DMA.h"

// Configurazione hardware
#define CS_PIN 5
#define DRDY_PIN 4

// Configurazione acquisizione
#define DEFAULT_SAMPLE_RATE 30000  // Hz
#define BATCH_PERIOD_US 5000      // 5ms = 200Hz
#define QUEUE_SIZE 25              // Dimensione coda batch
#define MAX_SAMPLES_PER_BATCH 310u  // Aggiunto 'u' per unsigned

const char* WIFI_SSID = "D-Link-6A30CC";
const char* WIFI_PASSWORD = "FabSeb050770250368120110";

// Configurazione Wi-Fi
//const char* WIFI_SSID = "WebPocket-E280";
//const char* WIFI_PASSWORD = "dorabino.7468!";

//const char* WIFI_SSID = "sensori";
//const char* WIFI_PASSWORD = "sensori2019";

// Struttura configurazione
struct Config {
    uint32_t sampleRate;
    uint8_t gain;
    bool filterEnabled;
    float threshold;
    bool streaming;
};
/*
// Struttura per iBatchData {
    uint32_t timestamp;
    uint16_t count;
    uint8_t values[MAX_SAMPLES_PER_BATCH][3];  // 3 bytes per valore
};
*/
// Variabili globali
DualWebSocket ws;
Config globalConfig = {DEFAULT_SAMPLE_RATE, 1, false, 1000000, true};
float emaAlpha = 0.1;
QueueHandle_t batchQueue;

// Funzione per creare e inviare lo stato del sistema
void sendSystemStatus(WebSocketServer::WSClient* client = nullptr) {
    char statusBuffer[128];
    snprintf(statusBuffer, sizeof(statusBuffer), 
        "{\"type\":\"status\",\"samplerate\":%u,\"alfaema\":%.3f,\"streaming\":%s}", 
        globalConfig.sampleRate,
        emaAlpha,
        globalConfig.streaming ? "true" : "false"
    );
    
    if(client) {
        // Rispondi solo al client che ha inviato i dati
        ws.sendDataToClient(client->id, statusBuffer, strlen(statusBuffer));
    } else {
        ws.sendDataSync(statusBuffer, strlen(statusBuffer));
    }
}

// Handler eventi per il canale dati
void onDataEvent(WSEventType type, WebSocketServer::WSClient* client, 
                     uint8_t* data, size_t len, void* arg) {
    if(type == WS_EVT_CONNECT) {
        Serial.printf("Data client #%d connected from %s\n", 
            client->id, client->remoteIP);
    }else if(type == WS_EVT_DISCONNECT) {
        Serial.printf("Data client #%u disconnected\n", client->id);
    }else if(type == WS_EVT_ERROR) {
         Serial.printf("WebSocket client #%u error %u: %s\n", client->id, *((uint16_t*)arg), (char*)data);
    }else if(type == WS_EVT_PONG) {
       // Il client ha risposto al ping
        Serial.printf("[WS] Client #%u ha risposto al ping\n", client->id);
    }
}

// Handler eventi per il canale di controllo
void onControlEvent(WSEventType type, WebSocketServer::WSClient* client, uint8_t* data, size_t len, void* arg) {
    Serial.printf("\nControl Event Type: ");
    switch(type) {
        case WS_EVT_CONNECT:
            Serial.println("CONNECT");
            break;
        case WS_EVT_DISCONNECT:
            Serial.println("DISCONNECT");
            break;
        case WS_EVT_DATA:
            Serial.println("DATA");
            break;
        case WS_EVT_ERROR:
            Serial.println("ERROR");
            break;
        case WS_EVT_PONG:
            Serial.println("PONG");
            break;
        default:
            Serial.printf("UNKNOWN (%d)", type);
    }
    /*
    int32_t targetInterval = 1000000 / globalConfig.sampleRate;
    
    // Calcola esatto numero di campioni per batch
    uint16_t samplesPerBatch = (uint16_t)((globalConfig.sampleRate * BATCH_PERIOD_US) / 1000000);
    samplesPerBatch = std::min<uint16_t>(samplesPerBatch, MAX_SAMPLES_PER_BATCH);
    
    Serial.printf("Sample rate: %d Hz\n", globalConfig.sampleRate);
    Serial.printf("Target interval: %d us\n", targetInterval);
    Serial.printf("Expected samples per batch: %d\n", samplesPerBatch);
    Serial.println("\n--- Control Event ---");  // Inizio evento
    Serial.printf("Event type: %d, len: %d\n", type, len);
    */
    if(type == WS_EVT_CONNECT) {
        Serial.printf("Control client #%d connected from %s\n", 
            client->id, client->remoteIP);
        sendSystemStatus(client);
    }
    else if(type == WS_EVT_DISCONNECT) {
        Serial.printf("Control client #%u disconnected\n", client->id);
    }
    else if(type == WS_EVT_DATA) {
        Serial.println("Received data event");

        data[len] = 0;  // Aggiungi terminatore
        Serial.print("Received JSON: ");
        Serial.println((char*)data);

        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, (char*)data);
        
        if(error) {
            Serial.print("JSON parse failed: ");
            Serial.println(error.c_str());
            return;
        }

        Serial.println("JSON parsed successfully");
        bool configChanged = false;
        bool lastStreaming;

        if(doc.containsKey("samplerate")) {
            uint32_t newRate = doc["samplerate"].as<int>();
            Serial.printf("Found samplerate: %d\n", newRate);
            if(newRate > 0) {
                globalConfig.sampleRate = newRate;
                Serial.printf("Sample rate impostato a: %d\n", newRate);
                lastStreaming = globalConfig.streaming;
                globalConfig.streaming = false;
                xQueueReset(batchQueue);
                Serial.println("Queue reset");
                delay(10);
                globalConfig.streaming = lastStreaming;
                configChanged = true;
            }
        }

        if(doc.containsKey("alfaema")) {
            float newAlpha = doc["alfaema"].as<float>();
            Serial.printf("Found alfaema: %.3f\n", newAlpha);
            emaAlpha = newAlpha;
            Serial.printf("EMA alpha impostato a: %.2f\n", emaAlpha);
            configChanged = true;
        }

        if(doc.containsKey("streaming")) {
            bool newStreaming = doc["streaming"].as<bool>();
            Serial.printf("Found streaming: %d\n", newStreaming);
            globalConfig.streaming = newStreaming;
            Serial.printf("Streaming: %s\n", globalConfig.streaming ? "avviato" : "fermato");
            configChanged = true;
        }
        
        if(configChanged) {
            Serial.println("Config changed, sending status");
            sendSystemStatus();
        }
    }
    Serial.println("--- Event End ---");  // Fine evento
}

void printMemoryInfo() {
    // RAM heap
    uint32_t freeHeap = esp_get_free_heap_size();
    uint32_t minFreeHeap = esp_get_minimum_free_heap_size();
    
    // Memoria totale e libera con diverse capacità
    size_t freeDMA = heap_caps_get_free_size(MALLOC_CAP_DMA);
    size_t freeExec = heap_caps_get_free_size(MALLOC_CAP_EXEC);
    size_t free32 = heap_caps_get_free_size(MALLOC_CAP_32BIT);
    
    // Memoria PSRAM se disponibile
    size_t psramSize = 0;
    size_t freePSRAM = 0;
    if(psramFound()) {
        psramSize = ESP.getPsramSize();
        freePSRAM = ESP.getFreePsram();
    }
    
    // Stampa informazioni
    Serial.println("\nMemory Info:");
    Serial.printf("Free Heap: %u bytes\n", freeHeap);
    Serial.printf("Min Free Heap: %u bytes\n", minFreeHeap);
    Serial.printf("Free DMA: %u bytes\n", freeDMA);
    Serial.printf("Free Exec: %u bytes\n", freeExec);
    Serial.printf("Free 32-bit: %u bytes\n", free32);
    
    if(psramFound()) {
        Serial.printf("Total PSRAM: %u bytes\n", psramSize);
        Serial.printf("Free PSRAM: %u bytes\n", freePSRAM);
    } else {
        Serial.println("No PSRAM found");
    }

    // Informazioni sulla frammentazione
    Serial.printf("Largest free block: %d bytes\n", 
        heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}

// Funzione per testare l'allocazione di una certa dimensione
bool testAllocation(size_t size) {
    void* ptr = malloc(size);
    if(ptr) {
        free(ptr);
        return true;
    }
    return false;
}

// Funzione per trovare la dimensione massima allocabile
size_t findMaxAllocation() {
    size_t min = 0;
    size_t max = esp_get_free_heap_size();
    
    while(min < max) {
        size_t mid = min + ((max - min + 1) / 2);
        if(testAllocation(mid)) {
            min = mid;
        } else {
            max = mid - 1;
        }
    }
    
    return min;
}

uint16_t getDecimationFactor(uint32_t desiredRate) {
    switch(desiredRate) {
        case 200:   
            return 150;    // 30000/150 = 200Hz
        case 500:   
            return 60;     // 30000/60 = 500Hz
        case 1000:  
            return 30;     // 30000/30 = 1000Hz
        case 10000: 
            return 3;      // 30000/3 = 10000Hz
        case 15000: 
            return 2;      // 30000/2 = 15000Hz
        case 30000: 
            return 1;      // 30000/1 = 30000Hz (no decimation)
        default:
            return 1;      // Default: nessuna decimazione
    }
}

void adcTask(void* pvParameters) {
    ADS1256_DMA ads;
    BatchData batch;
    uint32_t lastSample = 0;
    uint32_t overcount = 0;

    // Calcola parametri di batch
    uint32_t targetInterval = 5000;
    uint16_t samplesPerBatch = (uint16_t)((globalConfig.sampleRate * BATCH_PERIOD_US) / 1000000);
    samplesPerBatch = std::min<uint16_t>(samplesPerBatch, MAX_SAMPLES_PER_BATCH);
    uint16_t decimationFactor = getDecimationFactor(globalConfig.sampleRate);
    
    Serial.printf("Sample rate: %d Hz\n", globalConfig.sampleRate);
    Serial.printf("Target interval: %d us\n", targetInterval);
    Serial.printf("Expected samples per batch: %d\n", samplesPerBatch);

    // Attendi connessione WiFi
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    delay(1000);

    while (true) {
        if (!globalConfig.streaming) {
            samplesPerBatch = (uint16_t)((globalConfig.sampleRate * BATCH_PERIOD_US) / 1000000);
            samplesPerBatch = min(samplesPerBatch, (uint16_t)MAX_SAMPLES_PER_BATCH);
            ads.setEMAalfa(emaAlpha);
            
            Serial.printf("Blocco task: %d Hz\n", globalConfig.sampleRate);
            Serial.printf("targetInterval: %d\n", targetInterval);
            Serial.printf("samplesPerBatch: %d\n", samplesPerBatch);
            
            lastSample = 0;
            xQueueReset(batchQueue);
            
            Serial.println("Queue reset");
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        uint32_t now = micros();
        if ((now - lastSample) >= targetInterval) {
            lastSample += targetInterval;

            //uint16_t decimationFactor = getDecimationFactor(globalConfig.sampleRate);
            ads.read_data_batch(batch, samplesPerBatch, decimationFactor);             
            if(batch.count > 0) {
                if (xQueueSend(batchQueue, &batch, 0) != pdTRUE) {
                    Serial.println("Queue overflow: " + String(overcount++));
                }
            }
        }
        
        // Piccola pausa per evitare di saturare la CPU
        portYIELD();
    }
}

void wsTask(void* pvParameters) {
    BatchData batch;
    char buffer[2800]; 

    const int MAX_LEN = sizeof(buffer);

    while (true) {
        if (xQueueReceive(batchQueue, &batch, portMAX_DELAY) == pdTRUE) {
            //if (ws.count() > 0) {
            // Inizia JSON
            int len = snprintf(buffer, MAX_LEN, 
                "{\"t\":%u,\"v\":[", batch.timestamp);
            
            // Controlla overflow
            if (len < 0 || len >= MAX_LEN) {
                Serial.println("Buffer overflow nel header");
                continue;
            }

            // Serializza come hex strings
            const uint16_t maxValues = std::min<uint16_t>(batch.count, MAX_SAMPLES_PER_BATCH);
            for(int i = 0; i < maxValues && len < (MAX_LEN - 16); i++) {  // 16 bytes di margine
                int added = snprintf(buffer + len, MAX_LEN - len,
                    "\"%02x%02x%02x\"%s", 
                    batch.values[i][0], batch.values[i][1], batch.values[i][2],
                    (i < maxValues - 1) ? "," : "");
                
                if (added < 0 || added >= (MAX_LEN - len)) {
                    Serial.println("Buffer overflow nei valori");
                    break;
                }
                len += added;
            }

            // Chiudi JSON
            int final = snprintf(buffer + len, MAX_LEN - len, "]}");
            if (final > 0 && (len + final) < MAX_LEN) {
                len += final;
                //ws.sendDataSync(buffer, len);
                //if (ws.hasDataClients()) {
                //ws.sendDataSync(buffer, len);
                //int max = findMaxAllocation();
                //Serial.printf("Max possible allocation %d\n", max);
                //printMemoryInfo();
                //if (len > max) {
                //    Serial.printf("Send failed: requested size %d larger than max possible allocation %d\n", len, max);
                //    return;
                //} 
                //ws.sendDataAsync(buffer, len);
                ws.sendDataSync(buffer, len);
                //}
                //Serial.println("Buffer");
            }
            //ws.sendDataAsync("{\"t\":74111471,\"v\":[\"ee2db6\",\"f2dad4\",\"f95db8\",\"02381d\",\"020a4c\",\"fe961b\",\"f29324\",\"fa7321\",\"03ae6a\",\"068c70\",\"0553e8\",\"056839\",\"03b660\",\"0bb73b\",\"0ab73d\",\"04beae\",\"f7f5bd\",\"f66406\",\"eac1e5\",\"ef3e3c\",\"e8fdf6\",\"eaeb4a\",\"e1aa24\",\"eac267\",\"e8b73e\",\"eeb306\",\"e71e5a\",\"e877a7\",\"ee93c4\",\"eb0990\",\"e7a9c8\",\"e63d17\",\"f4f073\",\"fe3d96\",\"f9c700\",\"02b540\",\"0bdc0f\",\"009c78\",\"fe752d\",\"f3d24a\",\"f0c061\",\"ec151c\",\"f04806\",\"f92c23\",\"f4e6b8\",\"021935\",\"006e2b\",\"f818f9\",\"f6bad9\",\"fb81b8\",\"0853a7\",\"125666\",\"14ad7f\",\"090fea\",\"11f91f\",\"0470ef\",\"02ed5c\",\"05d44d\",\"080707\",\"01d6ec\",\"014ca0\",\"0463b9\",\"ff278e\",\"04a9bb\",\"fe50e5\",\"f6ed86\",\"f2be0f\",\"f81aa0\",\"edf702\",\"f3a39a\",\"f45e86\",\"f46f30\",\"f12860\",\"f26d6f\",\"ffd194\",\"0a53d0\",\"09de15\",\"ff3af2\",\"f52b3e\",\"00a8b9\",\"ffb684\",\"04aa4b\",\"fd5626\",\"023073\",\"fc1a4a\",\"f30bed\",\"f3a763\",\"fd1046\",\"fa8ce1\",\"f2cd97\",\"ecd82b\",\"e8c2d0\",\"e7a69a\",\"ec9201\",\"e378bd\",\"e462e5\",\"ec87c1\",\"eada08\",\"ec3969\",\"ea98c4\",\"e10996\",\"d79670\",\"e75895\",\"df5748\",\"dceb73\",\"d8a124\",\"e10d33\",\"e63dea\",\"e0403d\",\"e752b3\",\"f00733\",\"e829a4\",\"f407ad\",\"e8730b\",\"f3caef\",\"f41d8d\",\"f307ce\",\"fede2e\",\"0a3df6\",\"0b3e1a\",\"011665\",\"f4e6d3\",\"fbde0e\",\"fef5e6\",\"027aca\",\"0728a4\",\"faf3a2\",\"f4e808\",\"fc779f\",\"03abbd\",\"fb98fa\",\"f551a8\",\"01f0ae\",\"07d303\",\"07a80d\",\"0b8668\"]}");     
            //ws.sendDataAsync("{\"t\":\"74111471\",\"v\":[\"ee2db6\",\"ea3cb6\"]}", strlen("{\"t\":\"74111471\",\"v\":[\"ee2db6\",\"ea3cb6\"]}"));
            //}
            //vTaskDelay(10);
        }else {
            vTaskDelay(1);  // delay solo se non ci sono dati
        }
    }
}

void setup() {
    Serial.begin(115200);
     // Inizializzazione WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnesso al WiFi");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    delay(1000);
    // Crea la coda per i batch
    batchQueue = xQueueCreate(QUEUE_SIZE, sizeof(BatchData));
    if (batchQueue == NULL) {
        Serial.println("Errore creazione coda batch");
        while(1);
    }
    delay(1000);
    // Configurazione WebSocket
    ws.onDataEvent(onDataEvent);
    ws.onControlEvent(onControlEvent);
    if (!ws.begin()) {
        Serial.println("Failed to start WebSocket servers");
        ESP.restart();
    }
    delay(1000);
    
    // Task WebSocket su core 0
    xTaskCreatePinnedToCore(
        wsTask,
        "WS Task",
        8192,  // Stack aumentato
        NULL,
        1,
        NULL,
        0
    );
    delay(100);

    // Task ADC su core 1
    xTaskCreatePinnedToCore(
        adcTask, 
        "ADC Task", 
        4096,  // Stack aumentato
        NULL, 
        configMAX_PRIORITIES - 1,
        NULL, 
        1
    );

    Serial.println("Setup completato");
}

void loop() {
    vTaskDelay(10);
}