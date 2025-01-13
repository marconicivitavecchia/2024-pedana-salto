
#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include "DualWebSocket.h"
#include "ADS1256_DMA.h"

// Configurazione hardware
#define CS_PIN 5
#define DRDY_PIN 4
#define TONE_PIN 5

// Configurazione acquisizione
#define DEFAULT_SAMPLE_RATE 30000  // Hz
#define BATCH_PERIOD_US 5000      // 5ms = 200Hz
#define QUEUE_SIZE 25              // Dimensione coda batch
#define MAX_SAMPLES_PER_BATCH 310u  // Aggiunto 'u' per unsigned

//const char* WIFI_SSID = "D-Link-6A30CC";
//const char* WIFI_PASSWORD = "FabSeb050770250368120110";

const char* WIFI_SSID = "RedmiSeb";
const char* WIFI_PASSWORD = "pippo2503";

// Configurazione Wi-Fi
//const char* WIFI_SSID = "WebPocket-E280";
//const char* WIFI_PASSWORD = "dorabino.7468!";

//const char* WIFI_SSID = "sensori";
//const char* WIFI_PASSWORD = "sensori2019";

// Struttura configurazione
struct Config {
    uint32_t sampleRate;
    uint8_t gain;
    bool streaming;
    uint8_t mode;
    uint16_t toneFreq;
    uint8_t adcPort;
};
/*
BatchData {
    uint32_t timestamp;
    uint16_t count;
    uint32_t values[MAX_SAMPLES_PER_BATCH][3];  // 3 bytes per valore
};
*/
// Variabili globali
DualWebSocket ws;
Config globalConfig = {DEFAULT_SAMPLE_RATE, 1, false, 0, 1, 1};
volatile bool last = false;
volatile bool curr = false;
volatile bool overflow;
volatile bool enable1 = true;
float emaAlpha = 0.1;
QueueHandle_t batchQueue;
SemaphoreHandle_t configMutex;

// Funzione per creare e inviare lo stato del sistema
void sendSystemStatus(Config gc, WebSocketServer::WSClient* client = nullptr) {
    Serial.print("sendSystemStatus");
    char statusBuffer[130];
    snprintf(statusBuffer, sizeof(statusBuffer), 
        "{\"type\":\"status\",\"samplerate\":%u,\"alfaema\":%.3f,\"streaming\":\"%s\",\"mode\":\"%u\",\"freq\":\"%u\"}", 
        gc.sampleRate, emaAlpha, gc.streaming ? "true" : "false", gc.mode, gc.toneFreq);
    
    if(client) {
        // Rispondi solo al client che ha inviato i dati
        ws.sendControlToClient(client->id, statusBuffer, strlen(statusBuffer));
    } else {
        ws.sendControlSync(statusBuffer, strlen(statusBuffer));
    }
}

void sendOverflowStatus(bool status, WebSocketServer::WSClient* client = nullptr) {
    char statusBuffer[30];
    snprintf(statusBuffer, sizeof(statusBuffer), 
        "{\"type\":\"event\",\"overflow\":%u}", status);
    
    if(client) {
        // Rispondi solo al client che ha inviato i dati
        ws.sendControlToClient(client->id, statusBuffer, strlen(statusBuffer));
    } else {
        ws.sendControlSync(statusBuffer, strlen(statusBuffer));
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
    Config gc1;
    if (xSemaphoreTake(configMutex, portMAX_DELAY) == pdTRUE) {
        gc1 = globalConfig;
        xSemaphoreGive(configMutex);
    }  
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
        Serial.printf("Control client #%d connected from %s\n", client->id, client->remoteIP);
        sendSystemStatus(gc1, client);
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

        if(doc.containsKey("samplerate")) {
            uint32_t newRate = doc["samplerate"].as<int>();
            Serial.printf("Found samplerate: %d\n", newRate);
            if(newRate > 0) {
                gc1.sampleRate = newRate;
                Serial.printf("Sample rate impostato a: %d\n", newRate);
                //curr = false; // arresto immediato del loop adc
                //last = curr;
                xQueueReset(batchQueue);
                Serial.println("Queue reset");
                delay(10);
                //curr = gc1.streaming; // riavvio immediato del loop adc
                //last = curr;
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

        if(doc.containsKey("freq")) {
            uint16_t newFreq = doc["freq"].as<uint16_t>();
            Serial.printf("Found freq: %d\n", newFreq);
            gc1.toneFreq = newFreq;
            Serial.printf("Freq: %d\n", gc1.toneFreq);
            configChanged = true;
        }

        if(doc.containsKey("mode")) {
            uint8_t newMode = doc["mode"].as<uint8_t>();
            Serial.printf("Found mode: %d\n", newMode);
            gc1.mode = newMode;
            if(gc1.mode == 2){
                gc1.gain = 1;
                gc1.adcPort = 5;
            }else{
                gc1.gain = 6;
                gc1.adcPort = 1;
            }
            Serial.printf("Modo: %u attivato\n", gc1.mode);
            configChanged = true;
        }

        if(doc.containsKey("streaming")) {
            bool newStreaming = doc["streaming"].as<bool>();
            Serial.printf("Found streaming: %d\n", newStreaming);
            gc1.streaming = newStreaming;
            Serial.printf("Streaming: %s\n", gc1.streaming ? "avviato" : "fermato");
            configChanged = true;
        }

        if(configChanged) {
            if (xSemaphoreTake(configMutex, portMAX_DELAY) == pdTRUE) {
                globalConfig = gc1;
                curr = gc1.streaming;
                xSemaphoreGive(configMutex);
            }  
            Serial.println("Config changed, sending status");
            sendSystemStatus(gc1, client);  
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
        case 600:   
            return 50;     // 30000/50 = 600Hz
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
    ADS1256_DMA adc;
    BatchData batch;
    Config gc;
    uint32_t lastSample = 0;
    uint32_t overcount = 0;
    // uint32_t lastOvercount = 0;
    // Per leggere
    if (xSemaphoreTake(configMutex, portMAX_DELAY) == pdTRUE) {
        gc = globalConfig;
        xSemaphoreGive(configMutex);
    }   
    // Calcola parametri di batch
    uint32_t targetInterval = 5000;
    uint16_t samplesPerBatch = (uint16_t)((gc.sampleRate * BATCH_PERIOD_US) / 1000000);
    samplesPerBatch = std::min<uint16_t>(samplesPerBatch, MAX_SAMPLES_PER_BATCH);
    uint16_t decimationFactor = getDecimationFactor(gc.sampleRate);
    
    Serial.printf("Sample rate: %d Hz\n", gc.sampleRate);
    Serial.printf("Target interval: %d us\n", targetInterval);
    Serial.printf("Expected samples per batch: %d\n", samplesPerBatch);
    // Imposta parametri del segnale: frequenza, ampiezza, frequenza AM
    adc.setTestSignalParams(1.0f, 1.25f, 0.1f);
    
    // Attendi connessione WiFi
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    delay(1000);

    while (true) {
        //Serial.println("curr: "+String(curr)+" last: "+String(last));
        if(last != curr){
            if (xSemaphoreTake(configMutex, portMAX_DELAY) == pdTRUE) {
                gc = globalConfig;
                xSemaphoreGive(configMutex);
            }   
            Serial.println("adcTask: Cambio stato stream");
            samplesPerBatch = (uint16_t)((gc.sampleRate * BATCH_PERIOD_US) / 1000000);
            samplesPerBatch = min(samplesPerBatch, (uint16_t)MAX_SAMPLES_PER_BATCH);
            adc.setEMAalfa(emaAlpha);
            decimationFactor = getDecimationFactor(gc.sampleRate);         
            Serial.printf("adcTask: Blocco task: %d Hz\n", gc.sampleRate);
            Serial.printf("adcTask: targetInterval: %d\n", targetInterval);
            Serial.printf("adcTask: samplesPerBatch: %d\n", samplesPerBatch);
            Serial.printf("adcTask: decimationFactor: %d\n", decimationFactor);
            lastSample = 0;
            xQueueReset(batchQueue);   
            Serial.println("adcTask: Queue reset");
            //vTaskDelay(pdMS_TO_TICKS(100));
            if (!gc.streaming) {
                // Alla fine dello streaming
                adc.stopStreaming();
                Serial.print("adcTask: stopStreaming ");
                Serial.println(gc.streaming);
            }else{
                overcount = 0;
                //lastOvercount = 1;
                overflow = false;
                enable1 = true;
                adc.setTestSignalParams(gc.toneFreq, 8388608.0f, 0.1f);
                adc.set_gain(static_cast<ads1256_gain_t>(gc.gain));
                adc.set_channel(static_cast<ads1256_channels_t>(gc.adcPort), static_cast<ads1256_channels_t>(gc.adcPort + 1));
                if(gc.mode == 2){
                    Serial.println("adcTask: start Tone");
                    tone(TONE_PIN, gc.toneFreq);
                }else{
                    Serial.println("adcTask: stop Tone");
                    noTone(TONE_PIN);
                }
                if(gc.mode == 1){
                    // Abilita il segnale di test
                    adc.enableTestSignal(true);
                    Serial.println("adcTask: Segnale di test abilitato");
                }else{
                    // Disabilita il segnale di test
                    adc.enableTestSignal(false);
                    Serial.println("adcTask: Segnale di test disabilitato");
                }   
                adc.startStreaming();
                Serial.print("adcTask: startStreaming");    
                Serial.println(gc.streaming);               
            }
            last = curr;
        }        

        uint32_t now = micros();
        if (curr && (now - lastSample) >= targetInterval) {
            lastSample = now;
            /*
            batch.values[0][0] = 0;
            batch.values[0][1] = 0;
            batch.values[0][2] = 0;
            batch.timestamp = now;
            batch.count = 150;
            */
            //uint16_t decimationFactor = getDecimationFactor(globalConfig.sampleRate);
            adc.read_data_batch(batch, samplesPerBatch, decimationFactor);             
            if(batch.count > 0) {
                if (xQueueSend(batchQueue, &batch, 0) != pdTRUE) {
                    //overcount++;
                    overflow = true;    // lastOvercount != overcount
                    //Serial.print("Queue overflow: " + String(overcount));
                }else{
                    overflow = false; // lastOvercount == overcount;
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

            //int len = snprintf(buffer, MAX_LEN, 
            //    "{\"t\":%u,\"first\":%u,\"v\":[", batch.timestamp, batch.first);
            
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
                //Serial.println(buffer);
                //}
                //Serial.println("Buffer");
            }
                         
        }else {     
            if(overflow){
                if(enable1){
                    enable1 = false;
                    Serial.println("wsTask: sendOverflow true");
                    sendOverflowStatus(true);
                }  
            }else{
                if(!enable1){
                    enable1 = true;
                    Serial.println("wsTask: sendOverflow false");
                    sendOverflowStatus(false);
                }  
            }     
            vTaskDelay(1);  // delay solo se non ci sono dati
        }
    }
}

void setup() {
    Serial.begin(115200);
    configMutex = xSemaphoreCreateMutex();
     // Inizializzazione WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    delay(1000);
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

    // Task ads su core 1
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