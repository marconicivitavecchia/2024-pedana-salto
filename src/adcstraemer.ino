#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include "DualWebSocket.h"
#include "ADS1256_DMA.h"
#include "TaskMonitor.h"
#include <driver/spi_master.h>
#include "ESP32WebServer.h"
#include "WiFiManager.h"
#include "Handlers.h"
//#include "ADS1256ZeroCopySerializer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "esp_log.h"
//#include "BinaryPacker.h"

//#include "ADS1256_BitBang.h"

#include "esp_task_wdt.h"
#include "esp_wifi.h"

#define ADS1256_DEBUG
//#include "esp32-hal-timer.h"

// Configurazione hardware
#define DAC1_PIN 25  // DAC1
#define DAC2_PIN 26  // DAC2

// Configurazione acquisizione
#define DEFAULT_SAMPLE_RATE 30000  // Hz
#define BATCH_PERIOD_US 5000       // 5ms = 200Hz
//#define QUEUE_SIZE 20              // Dimensione coda batch
//#define STREAM_BUFFER_SIZE (8192)          // 8KB per BatchData structures
//#define TRIGGER_LEVEL (sizeof(BatchData))  // Sveglia dopo 1 BatchData
//#define QUEUE_SIZE 200              // Dimensione coda batch
#define DATALEN 1372

//#define MAX_SAMPLES_PER_BATCH 160u  // Aggiunto 'u' per unsigned

//const char* WIFI_SSID = "D-Link-6A30CC";
//const char* WIFI_PASSWORD = "FabSeb050770250368120110";

// const char* WIFI_SSID = "RedmiSeb";
// const char* WIFI_PASSWORD = "pippo2503";

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
  uint16_t toneAmp;
};

/*
struct BatchData {
    uint32_t t;
    uint8_t count;
    uint8_t v[MAX_SAMPLES_PER_BATCH][3];  // 3 bytes per valore
};
*/
// Variabili globali
TaskMonitor* adcMonitor = nullptr;
//ADS1256_DMA* adc = nullptr;

DualWebSocket ws;
Config globalConfig = { DEFAULT_SAMPLE_RATE, 1, false, 0, 1, 1 };
volatile bool last = false;
volatile bool curr = false;
volatile bool overflow;
volatile uint8_t enable1 = 128;
float emaAlpha = 0.1;
//VirtualStringQueue batchQueue;
//StreamBufferHandle_t batchStream;
//QueueHandle_t batchQueue;
//CharCircularBuffer batchQueue(55, DATALEN);
// Dichiara il ring buffer come variabile globale
RingbufHandle_t batchQueue = NULL;
// Tag per logging (opzionale)
static const char* TAGBUF = "RINGBUF_APP";
SemaphoreHandle_t configMutex;
TaskHandle_t adcTaskHandle = NULL;
//hw_timer_t * timer = NULL;
//portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile float freq = 1;
volatile uint16_t amp = 200;
volatile uint32_t angle = 0;
volatile uint8_t timerCmd = 0;
volatile uint8_t lastTimerCmd = 0;
const uint32_t SAMPLES_PER_CYCLE = 200;  // 1000ms/5ms = 200 campioni per ciclo
//ADS1256_DMA adc;
ADS1256_DMA adc;
//ADS1256_BitBang adc1;
//HTTPSServer https_server;
ESP32WebServer server;
//BinaryPacker packer;

//100, 100
//25, 12
void updateDAC(uint8_t offset, uint8_t amplitude) {
  // Calcola il valore della sinusoide
  float sinValue = sin(2 * PI * angle / SAMPLES_PER_CYCLE);
  
  // Per 640mV picco-picco con media 320mV:
  // Media: 320mV = ~25 unità DAC (320/12.94)
  // Ampiezza: ±160mV = ~12 unità DAC (160/12.94)
  //int offset = 25;    // 320mV di offset
  //int amplitude = 12; // ±160mV di ampiezza
  
  // DAC1: sinusoide normale
  int value1 = offset + amplitude * sinValue;
  
  // DAC2: controfase
  int value2 = offset - amplitude * sinValue;
  
  // Limita i valori tra 0 e 255 per sicurezza
  value1 = constrain(value1, 0, 255);
  value2 = constrain(value2, 0, 255);
  
  // Aggiorna entrambi i DAC
  dacWrite(DAC1_PIN, value1);
  dacWrite(DAC2_PIN, value2);
  
  angle++;
  if (angle >= SAMPLES_PER_CYCLE) {
    angle = 0;
  }
}

/*
void ARDUINO_ISR_ATTR onTimer() {
    float f = 10;
    portENTER_CRITICAL_ISR(&timerMux);
    f = freq;
    portEXIT_CRITICAL_ISR(&timerMux); 
    uint32_t value = 100 + 50 * sin(angle);  
    //dacWrite(DAC_PIN, value);
    angle += 2 * PI * f / 1000;
    if(angle >= 2 * PI) angle -= 2 * PI;
}

void setupDAC(){
    Serial.println("setupDAC");
    timer = timerBegin(10000);  // 1kHz
    if (timer == NULL) {
        Serial.println("Errore creazione timer!");
        return;
    }
    Serial.println("Timer creato");

    // Stampa frequenza subito dopo la creazione
    float freq = timerGetFrequency(timer);00
    Serial.print("Frequenza iniziale timer: ");
    Serial.println(freq);

    timerAttachInterrupt(timer, &onTimer);  
    Serial.println("Interrupt attaccato");

    timerAlarm(timer, 1000, true, 0);
    Serial.println("Alarm impostato");

    timerWrite(timer, 0);
    Serial.println("Timer azzerato");

    // Stampa frequenza dopo la configurazione completa
    freq = timerGetFrequency(timer);
    Serial.print("Frequenza finale timer: ");
    Serial.println(freq);

    timerStop(timer);
    Serial.println("Timer fermato");
}

void startDAC() {
    if (timer != NULL) {
        //timerAlarm(timer, 1000, true, 0);
        //Serial.println("Alarm impostato");
        timerStart(timer);
        //Serial.print("DAC timer started with freq: ");
        //Serial.println(timerGetFrequency(timer));
    }
}

void stopDAC() {
    if (timer != NULL) {
        timerStop(timer);
        dacWrite(DAC1_PIN, 0);
        dacWrite(DAC2_PIN, 0);
        //Serial.println("DAC timer stopped");
    }
}
*/
// Funzione per creare e inviare lo stato del sistema
void sendSystemStatus(Config gc, WebSocketServer::WSClient* client = nullptr) {
  Serial.print("sendSystemStatus");
  char statusBuffer[150];
  snprintf(statusBuffer, sizeof(statusBuffer),
           "{\"type\":\"status\",\"samplerate\":%u,\"alfaema\":%.3f,\"streaming\":\"%s\",\"mode\":\"%u\",\"freq\":\"%u\"}",
           gc.sampleRate, emaAlpha, gc.streaming ? "true" : "false", gc.mode, gc.toneFreq);

  if (client) {
    // Rispondi solo al client che ha inviato i dati
    ws.sendControlToClient(client->id, statusBuffer, strlen(statusBuffer));
  } else {
    ws.sendControlSync(statusBuffer, strlen(statusBuffer));
  }
}

void sendOverflowStatus(bool status, WebSocketServer::WSClient* client = nullptr) {
  char statusBuffer[50];
  snprintf(statusBuffer, sizeof(statusBuffer),
           "{\"type\":\"event\",\"overflow\":%u}", status);

  if (client) {
    // Rispondi solo al client che ha inviato i dati
    ws.sendControlToClient(client->id, statusBuffer, strlen(statusBuffer));
  } else {
    ws.sendControlSync(statusBuffer, strlen(statusBuffer));
  }
}

// Handler eventi per il canale dati
void onDataEvent(WSEventType type, WebSocketServer::WSClient* client,
                 uint8_t* data, size_t len, void* arg) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("Data client #%d connected from %s\n",
                  client->id, client->remoteIP);
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("Data client #%u disconnected\n", client->id);
  } else if (type == WS_EVT_ERROR) {
    Serial.printf("WebSocket client #%u error %u: %s\n", client->id, *((uint16_t*)arg), (char*)data);
  } else if (type == WS_EVT_PONG) {
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
  switch (type) {
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
  if (type == WS_EVT_CONNECT) {
    Serial.printf("Control client #%d connected from %s\n", client->id, client->remoteIP);
    //sendSystemStatus(gc1, client);
    sendSystemStatus(gc1);
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("Control client #%u disconnected\n", client->id);
  } else if (type == WS_EVT_DATA) {
    Serial.println("Received data event");

    data[len] = 0;  // Aggiungi terminatore
    Serial.print("Received JSON: ");
    Serial.println((char*)data);

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, (char*)data);

    if (error) {
      Serial.print("JSON parse failed: ");
      Serial.println(error.c_str());
      return;
    }

    Serial.println("JSON parsed successfully");
    bool configChanged = false;

    if (doc.containsKey("samplerate")) {
      uint32_t newRate = doc["samplerate"].as<int>();
      Serial.printf("Found samplerate: %d\n", newRate);
      if (newRate > 0) {
        gc1.sampleRate = newRate;
        Serial.printf("Sample rate impostato a: %d\n", newRate);
        //curr = false; // arresto immediato del loop adc
        //last = curr;
        //xQueueReset(batchQueue);
        //Serial.println("Queue reset");
        //delay(10);
        //curr = gc1.streaming; // riavvio immediato del loop adc
        //last = curr;
        configChanged = true;
      }
    }

    if (doc.containsKey("alfaema")) {
      float newAlpha = doc["alfaema"].as<float>();
      Serial.printf("Found alfaema: %.3f\n", newAlpha);
      emaAlpha = newAlpha;
      Serial.printf("EMA alpha impostato a: %.2f\n", emaAlpha);
      configChanged = true;
    }

    if (doc.containsKey("freq")) {
      uint16_t newFreq = doc["freq"].as<uint16_t>();
      Serial.printf("Found freq: %d\n", newFreq);
      gc1.toneFreq = newFreq;
      Serial.printf("Freq: %d\n", gc1.toneFreq);
      configChanged = true;
    }

    if (doc.containsKey("mode")) {
      uint8_t newMode = doc["mode"].as<uint8_t>();
      Serial.printf("Found mode: %d\n", newMode);
      gc1.streaming = false;
      gc1.mode = newMode;
      gc1.gain = 6;
      gc1.adcPort = 0;
      if (gc1.mode == 2) {
        gc1.gain = 1;
        gc1.adcPort = 6;
      }else if (gc1.mode == 3) {
        gc1.gain = 6;
        gc1.adcPort = 6;
        Serial.printf("Modo: %u attivato\n", gc1.mode);
        configChanged = true;
	  }
    }

    if (doc.containsKey("streaming")) {
      bool newStreaming = doc["streaming"].as<bool>();
      Serial.printf("Found streaming: %d\n", newStreaming);
      gc1.streaming = newStreaming;
      Serial.printf("Streaming: %s\n", gc1.streaming ? "avviato" : "fermato");
      configChanged = true;
    }

    if (doc.containsKey("testamp")) {
      uint16_t newAmp = doc["streaming"].as<uint16_t>();
      Serial.printf("Found streaming: %d\n", newAmp);
      gc1.toneAmp = newAmp;
      Serial.printf("Streaming: %s\n", gc1.toneAmp ? "avviato" : "fermato");
      configChanged = true;
    }

    Serial.println("Config changed, sending status");
    //sendSystemStatus(gc1, client);
    sendSystemStatus(gc1);

    if (configChanged) {
      if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        globalConfig = gc1;
        curr = gc1.streaming;
        last = !curr;
        xSemaphoreGive(configMutex);
      } else {
        Serial.println("Timeout nel prendere il semaforo del task websocket");
      }
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
  if (psramFound()) {
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

  if (psramFound()) {
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
  if (ptr) {
    free(ptr);
    return true;
  }
  return false;
}

// Funzione per trovare la dimensione massima allocabile
size_t findMaxAllocation() {
  size_t min = 0;
  size_t max = esp_get_free_heap_size();

  while (min < max) {
    size_t mid = min + ((max - min + 1) / 2);
    if (testAllocation(mid)) {
      min = mid;
    } else {
      max = mid - 1;
    }
  }

  return min;
}

uint16_t getDecimationFactor(uint32_t desiredRate) {
  switch (desiredRate) {
    case 200:
      return 150;  // 30000/150 = 200Hz
    case 600:
      return 50;  // 30000/50 = 600Hz
    case 1000:
      return 30;  // 30000/30 = 1000Hz
    case 10000:
      return 3;  // 30000/3 = 10000Hz
    case 15000:
      return 2;  // 30000/2 = 15000Hz
    case 30000:
      return 1;  // 30000/1 = 30000Hz (no decimation)
    default:
      return 1;  // Default: nessuna decimazione
  }
}

void adcTask(void* pvParameters) {
  TaskMonitor* monitor = (TaskMonitor*)pvParameters;
  monitor->startTracking();
  Serial.println("ADC task: monitor acquisito");
  Serial.println("ADC task: Stop monentarly task tracking");
  float Vref = 3.3;
  //char buffer[1478];
  try {
		Serial.println("ADC task: prima ADS1256_DMA");
		// Verifichiamo lo stato prima della creazione
		//ADS1256_DMA adc;
		Serial.println("ADC task: dopo ADS1256_DMA");
		BatchData batch;
		Config gc;
		uint32_t lastSample = 0;
		//uint32_t overcount = 0;
		//uint32_t start, end, duration;
		// uint32_t lastOvercount = 0;
		// Per leggere
		Serial.println("ADC task: prima semaforo");
		if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
		  gc = globalConfig;
		  xSemaphoreGive(configMutex);
		} else {
		  Serial.println("Timeout nel prendere il primo semaforo");
		}
		Serial.println("ADC task: dopo semaforo");
		// Calcola parametri di batch
		uint32_t targetInterval = 5000;
		uint16_t samplesPerBatch = (uint16_t)((gc.sampleRate * BATCH_PERIOD_US) / 1000000);
		samplesPerBatch = std::min<uint16_t>(samplesPerBatch, MAX_SAMPLES_PER_BATCH);
		uint16_t decimationFactor = getDecimationFactor(gc.sampleRate);
		if(decimationFactor == 0){
			decimationFactor = 1;
		}
		Serial.printf("ADC task: Sample rate: %d Hz\n", gc.sampleRate);
		Serial.printf("ADC task: Target interval: %d us\n", targetInterval);
		Serial.printf("ADC task: Expected samples per batch: %d\n", samplesPerBatch);
		// Imposta parametri del segnale: frequenza, ampiezza, frequenza AM
		delay(100);
		adc.setTestSignalParams(1.0f, 1.25f, 0.1f);
		Serial.println("ADC task: Attendi connessione WiFi");
		delay(100);
		Serial.println("ADC task: Start task tracking");
	    uint32_t last_t = 0;
		uint8_t amp = 0;
		uint8_t ofst = 0;
		while (true) {
		    //Serial.println("curr: "+String(curr)+" last: "+String(last));
		    if (last != curr) {
				if (xSemaphoreTake(configMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
				  gc = globalConfig;
				  xSemaphoreGive(configMutex);
				} else {
				  Serial.println("ADC task: Timeout nel prendere il secondo semaforo");
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
				//Serial.println("adcTask: Queue reset");
				//xStreamBufferReset(batchStream);
				//xQueueReset(batchQueue);  
				//batchQueue.resetCharBuffer();		
				//queue.begin(55, DATALEN);
				reset_ringbuffer(batchQueue);
				recreate_ringbuffer(batchQueue, DATALEN*40);
				Serial.println("adcTask: Queue reset");
				if (!gc.streaming) {
				  // Alla fine dello streaming
				  delay(50);
				  adc.stopStreaming();
				  //adc1.stopStreaming();
				  Serial.print("adcTask: stopStreaming ");
				  Serial.println(gc.streaming);
				  adc.set_channel(static_cast<ads1256_channels_t>(gc.adcPort), static_cast<ads1256_channels_t>(gc.adcPort + 1));
				  Serial.print("adcTask: gain: ");
				  Serial.println(gc.gain);
				  adc.set_gain(static_cast<ads1256_gain_t>(gc.gain));
				  Serial.print("adcTask: ch1: ");
				  Serial.println(gc.adcPort);
				  Serial.print("adcTask: ch2: ");
				  Serial.println(gc.adcPort + 1);
				  adc.printDeviceStatus(adc.getDeviceStatus());
				  timerCmd = 0;
				} else {
				  //overcount = 0;
				  overflow = false;
				  enable1 = 127;
				  if (gc.mode == 3) {
					Serial.println("adcTask: start Tone");
					freq = gc.toneFreq;
					timerCmd = 1;
					ofst = 25;
					amp = 12;
					Serial.println("adcTask: enableTestSignal");
					adc.enableTestSignal(false);
					Serial.println("adcTask: dopo enableTestSignal");
				  }else if (gc.mode == 2) {
					Serial.println("adcTask: start Tone");
					freq = gc.toneFreq;
					timerCmd = 1;
					ofst = 100;
					amp = 100;
					Serial.println("adcTask: enableTestSignal");
					adc.enableTestSignal(false);
					Serial.println("adcTask: dopo enableTestSignal");
				  } else if (gc.mode == 1) {
					timerCmd = 0;
					adc.setTestSignalParams(gc.toneFreq, 8388608.0f, 0.1f);
					adc.enableTestSignal(true);
					Serial.println("adcTask: Segnale di test abilitato");
				  } else {
					//timerCmd = 0;
					timerCmd = 0;
					Serial.println("adcTask: stop Tone");
					adc.enableTestSignal(false);
					Serial.println("adcTask: Segnale di test disabilitato");
				  }
				  // azzera batch
				  Serial.print("adcTask: startStreaming: ");
				  memset(batch.v, 0, MAX_SAMPLES_PER_BATCH * 3);
				  delay(50);
				  adc.startStreaming();
				  //adc1.startStreaming();
				}
			    last = curr;
		    }

			monitor->heartbeat();
		    if (gc.streaming) {
				if (timerCmd) updateDAC(ofst, amp);
				
				char *buf = NULL;  // Puntatore che riceverà l'indirizzo
				if (batch.count > 0) {
					BaseType_t result = xRingbufferSendAcquire(batchQueue,(void**) &buf, (size_t) DATALEN, pdMS_TO_TICKS(0));
					if (result == pdTRUE && buf != NULL) {
						adc.read_data_batch_fast(batch, samplesPerBatch, decimationFactor);
						//uint32_t delta = batch.t - last_t;
						//last_t += delta;
						//Serial.print("delta: ");Serial.println(delta);
						//ADS1256_SERIALIZE_TO(batch, buffer);
						int len = serialize_fast(buf, batch, DATALEN);
						//Serial.println(buffer);
						xRingbufferSendComplete(batchQueue, (void*)buf);
						//ESP_LOGI(TAG, "Data sent immediately");
						overflow = false;
					} else {
						ESP_LOGW(TAGBUF, "Buffer full - dropping data");
						overflow = true; 
					}
				}
				
		    }
		    if (gc.streaming) {
			  taskYIELD();
		    } else {
			  vTaskDelay(1);
		    }
		}
  } catch (const std::exception& e) {
    Serial.printf("ADC task: eccezione - %s\n", e.what());
  } catch (...) {
    Serial.println("ADC task: eccezione sconosciuta");
  }

  Serial.println("ADC task: terminato");  // Non dovrebbe mai arrivare qui
}

int serialize(char*buf, BatchData &batch, int maxLen){
	int len = snprintf(buf, maxLen, "{\"t\":\"%llx\",\"v\":[", batch.t);

	// Serializza come hex strings
	const uint16_t maxValues = std::min<uint16_t>(batch.count, MAX_SAMPLES_PER_BATCH);
	for (int i = 0; i < maxValues && len < (maxLen - 16); i++) {  // 16 bytes di margine
		int added = snprintf(buf + len, maxLen - len,
							 "\"%02x%02x%02x\"%s",
							 batch.v[i][0], batch.v[i][1], batch.v[i][2],
							 (i < maxValues - 1) ? "," : "");
		len += added;
	}

	// Chiudi JSON
	int final = snprintf(buf + len, maxLen - len, "]}");
	len += final;	
	return len;
}

int serialize_fast(char* buf, BatchData &batch, int maxLen){
    static const char hex[] = "0123456789abcdef";
    
    if (!buf) return -1;
    
    // Header - usando %llx per timestamp a 64-bit
    int len = snprintf(buf, maxLen, "{\"t\":\"%llx\",\"v\":[", 
                       (unsigned long long)batch.t);
       
    char* p = buf + len;
    char* buf_end = buf + maxLen - 3;
    
    // Processa tutti i campioni disponibili, limitati dal buffer
    uint16_t processed = 0;
    const uint16_t total = std::min<uint16_t>(batch.count, MAX_SAMPLES_PER_BATCH);
    
    for (int i = 0; i < total && processed < total; i++) {
        // Controllo spazio: ogni valore richiede 8 char ("xxxxxx",)
        
        *p++ = '"';
        uint8_t* v = batch.v[i];
        
        // Unroll completo per massima velocità
        uint8_t b0 = v[0], b1 = v[1], b2 = v[2];
        *p++ = hex[b0 >> 4]; *p++ = hex[b0 & 0x0F];
        *p++ = hex[b1 >> 4]; *p++ = hex[b1 & 0x0F];  
        *p++ = hex[b2 >> 4]; *p++ = hex[b2 & 0x0F];
        
        *p++ = '"';
        if (i < total - 1) *p++ = ',';
        
        processed++;
    }
    
    *p++ = ']'; 
    *p++ = '}'; 
    *p = '\0';
    
    return p - buf;
}

void wsTask(void* pvParameters) {
	//BatchData batch;
	char* buf;
	//char buffer[1500];
	uint32_t startTime = 0;
	const uint32_t timeout = 8000;
	uint32_t batchCount = 0;
	//BinaryPacker bp;
	size_t length;
		
	while (true) {
		// Aspetta fino a 10 secondi per i dati
		void *buf = xRingbufferReceive(batchQueue, &length, pdMS_TO_TICKS(0)); //non bloccante
		if (buf) {
			batchCount++;
            //Serial.println((char*)buf);
			ws.sendDataSync((uint8_t*)buf, (size_t) strlen((char*)buf));
            vRingbufferReturnItem(batchQueue, (void*)buf);
			
			// overflow signalling management
			if(overflow){
				if(enable1 > 127){
					enable1 = 127;// A single batch in overflow condition indicates an overflow status
					Serial.println("wsTask: sendOverflow true - reset buffer");
					sendOverflowStatus(true);
					//xQueueReset(batchQueue);
					//batchQueue.resetCharBuffer();
					//batchQueue.reset();
					reset_ringbuffer(batchQueue);
				}  
			}else{
				if(enable1 < 128){
					enable1--;
					if(enable1 < 97){// 20 consecutive batches in normal condition are required to confirm normal status
						Serial.println("wsTask: sendOverflow false");
						sendOverflowStatus(false);
						enable1 = 128;
					}     
				}  
			}               
		}else {  
			vTaskDelay(1);  // delay solo se non ci sono dati
		}
		// se non riceve dati il loop adc potrebbe essere fermo
		//Serial.println("wsTask: non ci sono dati"); 
		if(millis() - startTime > timeout) {
			startTime += timeout;        
			if (adcMonitor != nullptr) {  // verifica che adcMonitor sia valido
				if (!adcMonitor->isAlive()) {
					Serial.println("ADC non risponde!");
					adcMonitor->restartTask();
				}else{
					Serial.print("ADC OK! - ");
				}                
			} else {
				Serial.println("ADC monitor non inizializzato!");
			}
			batchCount = batchCount * 1000 / timeout;
			Serial.print(" Batch per sec: "); Serial.println(batchCount);
			//if(batchCount < 20) 
			//    xQueueReset(batchQueue);
			batchCount = 0;
		}			
	} 
}

void reset_ringbuffer(RingbufHandle_t ring_buf) {
    size_t item_size;
    void *item;
    
    ESP_LOGI(TAG, "Svuotando ring buffer...");
    
    // Leggi tutto senza timeout fino a quando è vuoto
    while ((item = xRingbufferReceive(ring_buf, &item_size, 0)) != NULL) {
        vRingbufferReturnItem(ring_buf, item);
        ESP_LOGD(TAG, "Rimosso elemento di %d bytes", item_size);
    }
    
    ESP_LOGI(TAG, "Ring buffer svuotato completamente");
}

RingbufHandle_t recreate_ringbuffer(RingbufHandle_t old_ring_buf, size_t size) {
    // Elimina il vecchio buffer
    if (old_ring_buf != NULL) {
        vRingbufferDelete(old_ring_buf);
        ESP_LOGI(TAG, "Buffer precedente eliminato");
    }

    // Crea nuovo buffer
    RingbufHandle_t new_ring_buf = xRingbufferCreate(size, RINGBUF_TYPE_NOSPLIT);
    if (new_ring_buf != NULL) {
        ESP_LOGI(TAG, "Nuovo buffer creato - effettivamente resettato");
    }
    
    return new_ring_buf;
}

void setup() {
  //optimizeESP32();
  Serial.begin(115200);	
  //adc1.set_ema_alpha(0.05f);
  // Inizializza il filesystem
  Serial.println("\nInizializzazione filesystem");
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed");
    return;
  }
  Serial.println("\nInizializzazione ADC");
  //adc.start();
  configMutex = xSemaphoreCreateMutex();
  // Verifica memoria e stato buffer
  /*
  // Inizializzazione WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  delay(1000);
  */
  // *** OTTIMIZZAZIONE CON DUAL SSID ***
  //OPTIMIZE_WEBSOCKET_UNIVERSAL(
  //      "sensori", "sensori2019",     // Principale
  //      "RedmiSeb", "pippo2503"       // Backup
  //  );
  
  //SAVE_WIFI_CREDENTIALS("RedmiSeb", "pippo2503", "sensori", "sensori2019");
  
  // Esegui diagnostica completa
    //DIAGNOSE_NETWORK("RedmiSeb");
    
    // Inizializza con debug
    WiFiManager::optimizeEverything();
  
  SETUP_INTEGRATED_WIFI_FALLBACK(
		"RedmiSeb", "pippo2503",      
        "sensori", "sensori2019"    
   );
  
  Serial.println("\nConnesso al WiFi");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  String hostname = "adcstreamer-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  Serial.println("\nhostname: " + hostname);
  MDNS.begin(hostname.c_str());
  //setupDAC();
  delay(1000);
  // Inizializza la coda con 10 stringhe da 64 byte ciascuna
  //startDAC();
  // Crea la coda per i batch
  //batchQueue = xQueueCreate(QUEUE_SIZE, (uint8_t) DATALEN);
  //if (batchQueue == NULL) {
  //    Serial.println("Errore creazione coda batch");
  //    while(1);
  //}
/*
  batchStream = xStreamBufferCreate(STREAM_BUFFER_SIZE, TRIGGER_LEVEL);
  if (batchStream == NULL) {
    Serial.println("Errore creazione stream buffer");
    while (1)
      ;
  }
  Serial.printf("Batch stream buffer creato: %d bytes\n", STREAM_BUFFER_SIZE);
  */
  
  delay(1000);
  // Configurazione di default (porta 80)
  // Oppure con configurazione personalizzata
  // ESP32WebServer::Config config;
  // config.port = 8080;
  // config.check_proxy_headers = true;
  if (!server.begin()) {
    Serial.println("Server failed to start");
    ESP.restart();
  }
  
  // loaders pagine statiche da file
  server.loadFile("/","index.html");
  server.loadFile("/wifi","wifi.html");
  // loaders API 
  setupWiFiAPIHandlers(server);

  delay(100);  // Breve pausa per stabilizzazione

  // Configura e avvia il WebSocket server
  ws.onDataEvent(onDataEvent);
  ws.onControlEvent(onControlEvent);
  if (!ws.begin()) {
    ESP_LOGE("MAIN", "Failed to start WebSocket servers");
    ESP.restart();
  }
  delay(100);
  adc.begin();
  delay(100);
  
  check_heap_usage(DATALEN*40);
  
  // Task WebSocket su core 0
  xTaskCreatePinnedToCore(
    wsTask,
    "WS Task",
    8192,  // Stack aumentato
    NULL,
    1,
    NULL,
    0);
  /*
    // Task ads su core 1
    xTaskCreatePinnedToCore(
        adcTask, 
        "ADC Task", 
        5000,  // Stack aumentato
        NULL, 
        configMAX_PRIORITIES - 1,
        &adcTaskHandle, 
        1
    );
     */
  delay(1000);
  adcMonitor = new TaskMonitor(
    "ADC_Task",
    4096,  // timeout 2 secondi
    adcTask,
    nullptr,
    1,                         // core 1
    6000,                      // stack
    configMAX_PRIORITIES - 1,  // priority
    adcMonitor                 // il monitor stesso
  );
  // Settiamo il parametro dopo la creazione
  adcMonitor->setTaskParams(adcMonitor);
  adcMonitor->startTask();

  Serial.println("Setup completato");
}

void loop() {
  /*
    eTaskState taskState = eTaskGetState(adcTaskHandle);
    Serial.print("Stato task ADC: ");
    switch(taskState) {
        case eRunning:
            Serial.println("RUNNING");
            break;
        case eBlocked:
            Serial.println("BLOCKED");
            break;
        case eSuspended:
            Serial.println("SUSPENDED");
            break;
        case eDeleted:
            Serial.println("DELETED");
            break;
        case eReady:
            Serial.println("READY");
            break;
        default:
            Serial.println("STATO SCONOSCIUTO");
    }

    // Aggiungi anche info sullo stack
    UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(adcTaskHandle);
    Serial.printf("Stack libero: %d bytes\n", stackHighWaterMark);
   
    
    static uint32_t lastTime = 0;
    uint32_t now = millis();

    // Stampa stato ogni secondo
    if (now - lastTime >= 1000) {
        Serial.println("Loop running...");
        Serial.print(", "+timerCmd);
        lastTime = now;
    }

    if(timerCmd != lastTimerCmd){
        switch(timerCmd) {
            case 1:
                Serial.println("loop: startDAC");
                startDAC();
            break;
            case 0:
                Serial.println("loop: stopDAC");
                stopDAC();
            break;
        }
        lastTimerCmd = timerCmd;
    } */
  // Ora possiamo controllare isAlive() dal loop
  // Controllo ogni 2 minuti per switch intelligente
    static uint32_t lastCheck = 0;
    if (millis() - lastCheck > 120000) {
		CHECK_NETWORK_SWITCH();  // Controlla se c'è un AP migliore
		FIX_WIFI_UNIVERSAL();
		lastCheck = millis();
	}
  //vTaskDelay(pdMS_TO_TICKS(100));
  //vTaskDelay(10);
}

void setupWiFiAPIHandlers(ESP32WebServer& webServer) {
    // Configurazione WiFi (GET, POST, DELETE + OPTIONS per CORS)
    webServer.addHandler("/api/wifi/config", HTTP_GET, apiWiFiConfigGet);
    webServer.addHandler("/api/wifi/config", HTTP_POST, apiWiFiConfigPost);
    webServer.addHandler("/api/wifi/config", HTTP_DELETE, apiWiFiConfigDelete);  // ← MANCAVA
    webServer.addHandler("/api/wifi/config", HTTP_OPTIONS, apiWiFiConfigPost);   // ← MANCAVA (CORS)
    
    // Test WiFi
    webServer.addHandler("/api/wifi/test", HTTP_POST, apiWiFiTest);
    webServer.addHandler("/api/wifi/test", HTTP_OPTIONS, apiWiFiTest);           // ← MANCAVA (CORS)
    
    // Scan reti WiFi
    webServer.addHandler("/api/wifi/scan", HTTP_GET, apiWiFiScan);
    
    // Status sistema  
    webServer.addHandler("/api/system/status", HTTP_GET, apiSystemStatus);      // ← MANCAVA
}

void check_heap_usage(int dim) {
    // Memoria heap prima della creazione
    size_t heap_before = esp_get_free_heap_size();
    ESP_LOGI(TAG, "Heap libero prima: %d bytes", heap_before);
    
    // Crea ring buffer
    batchQueue = xRingbufferCreate(dim, RINGBUF_TYPE_NOSPLIT);
    
    // Memoria heap dopo la creazione
    size_t heap_after = esp_get_free_heap_size();
    ESP_LOGI(TAG, "Heap libero dopo: %d bytes", heap_after);
    
    size_t used = heap_before - heap_after;
    ESP_LOGI(TAG, "Memoria utilizzata: %d bytes", used);
    // Risultato: ~2048 + overhead (strutture interne)
    
    if (batchQueue != NULL) {
        ESP_LOGI(TAG, "✅ Ring buffer allocato con successo sull'heap");
    }
}

