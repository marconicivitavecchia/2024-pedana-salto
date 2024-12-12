>[Torna all'indice](readme.md#fasi-progetto)


### **Condizioni di Bernstein**

Le condizioni si basano sui concetti di insiemi di dati **letti** (read set) e **scritti** (write set) da ciascun processo:

- **Read set (R)**: insieme dei dati che il processo legge.
- **Write set (W)**: insieme dei dati che il processo scrive.

Per due processi \( P_1 \) e \( P_2 \), non ci sono interferenze (quindi possono essere eseguiti in parallelo) se sono soddisfatte le seguenti condizioni:

1. \( W_1 \cap W_2 = \emptyset \): i processi non scrivono sugli stessi dati.
2. \( W_1 \cap R_2 = \emptyset \): il primo processo non scrive dati che il secondo legge.
3. \( R_1 \cap W_2 = \emptyset \): il secondo processo non scrive dati che il primo legge.

In altre parole:
- I dati scritti da un processo non devono essere letti o scritti dall'altro.
- I dati letti da un processo non devono essere scritti dall'altro.

---

### **Esempio Pratico**
Immagina di avere due processi \( P_1 \) e \( P_2 \):
- \( P_1 \): legge \( A \) e scrive \( B \).
- \( P_2 \): legge \( B \) e scrive \( C \).

- \( R_1 = \{ A \}, W_1 = \{ B \} \)
- \( R_2 = \{ B \}, W_2 = \{ C \} \)

Verifica delle condizioni:
1. \( W_1 \cap W_2 = \emptyset \): \( \{ B \} \cap \{ C \} = \emptyset \) ✅
2. \( W_1 \cap R_2 = \emptyset \): \( \{ B \} \cap \{ B \} \neq \emptyset \) ❌
3. \( R_1 \cap W_2 = \emptyset \): \( \{ A \} \cap \{ C \} = \emptyset \) ✅

In questo caso, i due processi **non possono essere eseguiti in parallelo** perché \( W_1 \cap R_2 \neq \emptyset \): il primo processo scrive \( B \), che il secondo processo legge.

---

### **Applicazione in Sistemi Concreti**
Le condizioni di Bernstein sono utili per:
- Verificare la possibilità di **parallelizzare task** senza introdurre condizioni di corsa.
- Progettare sistemi in cui i dati condivisi sono protetti tramite tecniche come **mutex**, **sezioni critiche** o **variabili atomiche**.

Nel tuo caso con il buffer circolare:
- L'ISR scrive i dati nel buffer.
- Il task WebSocket legge i dati dal buffer.
  
Verifichiamo:
- L'ISR scrive \( W_{\text{ISR}} = \{\text{buffer}, \text{writeIndex}, \text{bufferCount}\} \).
- Il task WebSocket legge \( R_{\text{WebSocket}} = \{\text{buffer}, \text{readIndex}, \text{bufferCount}\} \) e scrive \( W_{\text{WebSocket}} = \{\text{readIndex}, \text{bufferCount}\} \).

In questo caso, \( W_{\text{ISR}} \cap R_{\text{WebSocket}} = \{\text{bufferCount}\} \), quindi c'è un'interferenza, e serve una protezione (ad esempio un **mutex** o sezioni critiche).

---

Ecco una tabella che riassume chiaramente le interferenze tra i due task basata sulle **condizioni di Bernstein**. Ho incluso riferimenti alla teoria per chiarezza.

---

### **Tabella delle Interferenze (Task ISR e WebSocket)**

| **Dati condivisi** | **Accesso ISR**      | **Accesso WebSocket** | **Interferenza** | **Violazione Bernstein**              |
|---------------------|----------------------|------------------------|-------------------|---------------------------------------|
| `buffer`           | **Scrittura**       | **Lettura**            | **Sì**           | \( W_{\text{ISR}} \cap R_{\text{WebSocket}} \neq \emptyset \) |
| `writeIndex`       | **Scrittura**       | **Nessun accesso**     | **No**           | Nessuna interferenza                  |
| `readIndex`        | **Nessun accesso**  | **Scrittura**          | **No**           | Nessuna interferenza                  |
| `bufferCount`      | **Scrittura**       | **Lettura e Scrittura**| **Sì**           | \( W_{\text{ISR}} \cap (R_{\text{WebSocket}} \cup W_{\text{WebSocket}}) \neq \emptyset \) |

---

### **Spiegazione**
1. **buffer**:
   - ISR scrive i dati acquisiti nel buffer.
   - Il task WebSocket legge i dati per trasmetterli.
   - **Interferenza**: Il dato scritto dall'ISR potrebbe essere letto contemporaneamente dal WebSocket, causando incoerenza nei dati.

2. **writeIndex**:
   - ISR aggiorna l'indice di scrittura del buffer.
   - Il task WebSocket non accede mai a `writeIndex`.
   - **Nessuna interferenza**: Non c'è condivisione.

3. **readIndex**:
   - ISR non accede mai a `readIndex`.
   - Il task WebSocket aggiorna l'indice di lettura.
   - **Nessuna interferenza**: Non c'è sovrapposizione.

4. **bufferCount**:
   - ISR aggiorna il contatore del numero di elementi nel buffer.
   - Il task WebSocket legge e scrive lo stesso contatore per verificare i dati disponibili.
   - **Interferenza**: L'accesso simultaneo a `bufferCount` può causare condizioni di corsa.

---

### **Violazioni delle Condizioni di Bernstein**
Le violazioni si manifestano nei seguenti punti:
1. **buffer**: \( W_{\text{ISR}} \cap R_{\text{WebSocket}} \neq \emptyset \).
2. **bufferCount**: \( W_{\text{ISR}} \cap (R_{\text{WebSocket}} \cup W_{\text{WebSocket}}) \neq \emptyset \).

Queste interferenze richiedono l'uso di:
- **Mutex**: per proteggere l'accesso al `buffer` e a `bufferCount`.
- **Code (queue)**: alternativa per gestire la comunicazione tra ISR e task WebSocket in modo sicuro.

---

### **Soluzioni Proposte**
1. **Protezione con Mutex**:
   - Usare un mutex per garantire che l'accesso al buffer e al contatore sia esclusivo.
   - Esempio: `xSemaphoreTake` e `xSemaphoreGive` di FreeRTOS.

2. **Sostituzione con Queue**:
   - Usare una coda per eliminare l'accesso diretto al `buffer` e a `bufferCount`.
   - L'ISR scrive nella coda, e il WebSocket legge dalla coda.

---






Ecco una versione del sistema che utilizza una **coda** per gestire la comunicazione tra l'ISR (che acquisisce i dati) e il task WebSocket, con i due task distribuiti su **core diversi**. La coda elimina le interferenze dirette sui dati condivisi, rendendo il sistema thread-safe.

---

### **Implementazione in MicroPython con FreeRTOS**

#### **Caratteristiche principali:**
1. **Uso di una coda**: La coda (`Queue`) viene utilizzata per passare i dati dal task ISR al task WebSocket in modo sicuro.
2. **Task su core diversi**:
   - Il task ISR gira sul **Core 0**.
   - Il task WebSocket gira sul **Core 1**.
3. **Acquisizione continua**: Il buffer della coda funge da memoria tampone per gestire eventuali ritardi di trasmissione.

---

### **Codice**

```python
from machine import SPI, Pin
from queue import Queue
from _thread import start_new_thread
import time

# Configurazione hardware
CS_PIN = 5
DRDY_PIN = 4

# Configurazione acquisizione
QUEUE_SIZE = 1024  # Dimensione della coda per buffering
SAMPLE_RATE = 30000  # Hz teorici dell'ADS1256

# Coda per la comunicazione tra ISR e WebSocket
data_queue = Queue(QUEUE_SIZE)

# Flag di controllo
acquiring = True

# ISR per l'acquisizione dati
def adc_isr():
    """Simula l'acquisizione dall'ADC."""
    global acquiring
    spi = SPI(1, baudrate=10_000_000, polarity=0, phase=1, sck=Pin(18), mosi=Pin(23), miso=Pin(19))
    cs = Pin(CS_PIN, Pin.OUT, value=1)
    drdy = Pin(DRDY_PIN, Pin.IN)
    
    while acquiring:
        if not drdy.value():  # Simula interrupt DRDY
            cs.off()
            spi.write(b'\x01')  # Comando RDATA
            data = spi.read(3)
            cs.on()

            # Converti i dati in valore numerico
            value = (data[0] << 16) | (data[1] << 8) | data[2]
            if value & 0x800000:
                value -= 0x1000000

            # Inserisci nella coda
            if not data_queue.full():
                data_queue.put(value)
            else:
                print("Coda piena! Dato perso.")
            
            # Simula la frequenza di campionamento
            time.sleep(1 / SAMPLE_RATE)

# Task WebSocket per lo streaming
def websocket_task():
    """Gestisce lo streaming WebSocket."""
    global acquiring

    # Simula un server WebSocket
    print("WebSocket Server avviato")
    while acquiring:
        if not data_queue.empty():
            value = data_queue.get()
            # Invia i dati (simulazione)
            print(f"Invio valore: {value}")
        else:
            time.sleep_ms(10)

# Funzione principale
def main():
    global acquiring
    
    print("Inizializzazione...")
    
    # Avvia l'ISR su Core 0
    print("Avvio acquisizione ADC su Core 0")
    start_new_thread(adc_isr, ())

    # Avvia il WebSocket su Core 1
    print("Avvio server WebSocket su Core 1")
    start_new_thread(websocket_task, ())

    # Simula acquisizione per un certo tempo
    time.sleep(10)  # Cambia con la durata desiderata
    acquiring = False  # Ferma acquisizione

    print("Sistema arrestato.")

if __name__ == "__main__":
    main()
```

---

### **Distribuzione dei Task**
1. **Core 0**:
   - ISR (acquisizione dati ADC): legge continuamente i dati dall'ADS1256 e li mette nella coda.
2. **Core 1**:
   - WebSocket: legge dalla coda e invia i dati al client.

---

### **Punti di forza**
- **Thread-Safe**: La coda garantisce che non ci siano condizioni di corsa (race conditions).
- **Prestazioni ottimizzate**: La separazione dei core sfrutta al massimo le capacità dell'ESP32.
- **Buffering**: La coda funge da memoria tampone per gestire ritardi temporanei nella trasmissione.

---

### **Considerazioni**
- La dimensione della coda (`QUEUE_SIZE`) dovrebbe essere sufficientemente grande per gestire il buffering in caso di ritardi.
- La frequenza di campionamento (30kHz) può essere sostenuta dall'ESP32, ma è importante verificare che il task WebSocket non introduca ritardi eccessivi.
- Potrebbe essere utile abilitare il **WiFi Power Save Mode** per ottimizzare l'efficienza energetica.

## **Versione in C++**

In MicroPython, purtroppo, non hai un controllo diretto sui **core** dell'ESP32, poiché il threading con `_thread` non consente di specificare esplicitamente su quale core eseguire un determinato thread. Tuttavia, FreeRTOS, il sistema operativo sottostante dell'ESP32, permette di legare i task ai core.

Per avere un controllo esplicito sui core, è necessario utilizzare **FreeRTOS** direttamente. Con MicroPython, l'approccio migliore è usare le funzionalità di **ESP-IDF** (la toolchain di Espressif) per configurare task specifici con il binding al core. Se desideri continuare a lavorare in MicroPython, puoi controllare indirettamente il comportamento sfruttando alcune caratteristiche:

---

### **Soluzione con FreeRTOS in C (binding diretto ai core)**
Se vuoi essere assolutamente certo che i thread siano distribuiti su due core, devi programmare in C utilizzando ESP-IDF. Ecco un esempio di come fare:

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include <stdio.h>

// Variabili globali
volatile bool acquiring = true;

// Task ISR (Core 0)
void adc_task(void *pvParameters) {
    while (acquiring) {
        // Simula acquisizione ADC
        printf("Acquisizione dati su Core 0\n");
        vTaskDelay(pdMS_TO_TICKS(1)); // Simula il tempo di acquisizione
    }
    vTaskDelete(NULL);
}

// Task WebSocket (Core 1)
void websocket_task(void *pvParameters) {
    while (acquiring) {
        // Simula invio WebSocket
        printf("Invio dati su Core 1\n");
        vTaskDelay(pdMS_TO_TICKS(10)); // Simula tempo di invio
    }
    vTaskDelete(NULL);
}

void app_main() {
    printf("Inizializzazione...\n");

    // Creazione dei task sui rispettivi core
    xTaskCreatePinnedToCore(adc_task, "ADC Task", 2048, NULL, 1, NULL, 0); // Core 0
    xTaskCreatePinnedToCore(websocket_task, "WebSocket Task", 2048, NULL, 1, NULL, 1); // Core 1

    // Simula esecuzione per 10 secondi
    vTaskDelay(pdMS_TO_TICKS(10000));
    acquiring = false;

    printf("Sistema arrestato.\n");
}
```

---

### **Distribuzione dei Core**
- `xTaskCreatePinnedToCore`: consente di associare un task a un core specifico.
  - `Core 0`: ADC (ISR).
  - `Core 1`: WebSocket.

---

### **Simulazione in MicroPython**
MicroPython non permette di controllare i core in maniera diretta, ma puoi utilizzare il modulo `_thread` per simulare un'esecuzione parallela:

1. **Verifica indiretta del comportamento**:
   - Includi un output che identifichi il core corrente:
     ```python
     import esp
     print(f"Core corrente: {esp.osdebug(None)}")
     ```
   - Questo comando può darti un'idea su quale core viene eseguito il thread (se supportato dalla tua build di MicroPython).

2. **Approccio alternativo**:
   - Se necessario, scrivi il task critico (ad esempio, ADC ISR) in C con FreeRTOS e utilizza un wrapper Python per interfacciarti con il resto del codice.

---

## **Versione più arduinizzata**

Ecco il codice tradotto in C/C++ per Arduino, con un approccio basato su **FreeRTOS** (disponibile nell'ambiente di Arduino per ESP32) per garantire che i due task vengano eseguiti su core diversi.

---

### **Caratteristiche principali**
1. **Distribuzione dei task**:
   - Il task ISR (acquisizione ADC) è fissato su **Core 0**.
   - Il task WebSocket (invio dati) è fissato su **Core 1**.
2. **Uso di una coda**: La coda (`QueueHandle_t`) viene utilizzata per passare i dati dal task ISR al task WebSocket in modo thread-safe.
3. **Frequenza di campionamento simulata**: L'acquisizione dei dati simula il funzionamento dell'ADS1256.

---

### **Codice**

```cpp
#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebSocketsServer.h>

// Configurazione hardware
#define CS_PIN 5
#define DRDY_PIN 4

// Configurazione coda e acquisizione
#define QUEUE_SIZE 1024  // Dimensione della coda
#define SAMPLE_RATE 30000 // Frequenza di campionamento (simulata in Hz)

// Variabili globali
QueueHandle_t dataQueue;
volatile bool acquiring = true;

// Configurazione WiFi e WebSocket
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
WebSocketsServer webSocket(81);

// Funzioni WebSocket
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_CONNECTED) {
    Serial.printf("Client %u connesso\n", num);
  } else if (type == WStype_DISCONNECTED) {
    Serial.printf("Client %u disconnesso\n", num);
  }
}

// Task per acquisizione ADC
void adcTask(void* pvParameters) {
  SPIClass spi(VSPI);
  spi.begin(18, 19, 23, CS_PIN);
  pinMode(CS_PIN, OUTPUT);
  pinMode(DRDY_PIN, INPUT);

  while (acquiring) {
    if (digitalRead(DRDY_PIN) == LOW) {
      // Simula lettura dati ADC
      digitalWrite(CS_PIN, LOW);
      uint8_t data[3] = {0x01, 0x02, 0x03}; // Valore simulato
      digitalWrite(CS_PIN, HIGH);
      int32_t value = (data[0] << 16) | (data[1] << 8) | data[2];
      if (value & 0x800000) value -= 0x1000000;

      // Inserisci nella coda
      if (xQueueSend(dataQueue, &value, 0) != pdTRUE) {
        Serial.println("Coda piena! Dato perso.");
      }

      // Simula frequenza di campionamento
      delayMicroseconds(1000000 / SAMPLE_RATE);
    }
  }

  vTaskDelete(NULL); // Termina il task
}

// Task per gestione WebSocket
void webSocketTask(void* pvParameters) {
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  while (acquiring) {
    int32_t value;
    if (xQueueReceive(dataQueue, &value, 1000 / portTICK_PERIOD_MS) == pdTRUE) {
      // Invia il dato via WebSocket
      String message = String(value);
      webSocket.broadcastTXT(message);
    }
    webSocket.loop();
  }

  vTaskDelete(NULL); // Termina il task
}

// Setup iniziale
void setup() {
  Serial.begin(115200);

  // Configurazione WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connesso");

  // Inizializzazione coda
  dataQueue = xQueueCreate(QUEUE_SIZE, sizeof(int32_t));
  if (dataQueue == NULL) {
    Serial.println("Errore nella creazione della coda!");
    while (true);
  }

  // Creazione dei task sui rispettivi core
  xTaskCreatePinnedToCore(adcTask, "ADC Task", 2048, NULL, 1, NULL, 0); // Core 0
  xTaskCreatePinnedToCore(webSocketTask, "WebSocket Task", 2048, NULL, 1, NULL, 1); // Core 1
}

// Ciclo principale
void loop() {
  // Se necessario, puoi aggiungere logica per fermare l'acquisizione
  delay(1000);
}
```

---

### **Punti salienti**
1. **Task ISR**:
   - Simula l'acquisizione di dati dall'ADS1256.
   - I dati vengono scritti in una coda (thread-safe).
2. **Task WebSocket**:
   - Legge i dati dalla coda e li invia a tutti i client connessi al server WebSocket.
3. **Distribuzione dei core**:
   - `xTaskCreatePinnedToCore` assicura che ogni task venga eseguito su un core specifico.

---

### **Considerazioni**
- La **frequenza di campionamento** (30 kHz) è gestita con `delayMicroseconds` nel task ISR.
- La **dimensione della coda** (`QUEUE_SIZE`) deve essere regolata in base ai ritardi previsti nella trasmissione.
- L'ESP32 può gestire l'acquisizione e il WebSocket simultaneamente grazie alla separazione dei core.

## **MQTT**

Sì, puoi utilizzare **MQTT** per inviare uno stream di letture ADC e, in alcuni casi, può essere altrettanto conveniente o addirittura migliore rispetto ai WebSocket, a seconda dello scenario specifico. Ecco una comparazione tra i due protocolli e un'implementazione MQTT per il tuo caso.

---

### **MQTT vs WebSocket**
| **Aspetto**             | **MQTT**                                                                 | **WebSocket**                                                     |
|--------------------------|-------------------------------------------------------------------------|-------------------------------------------------------------------|
| **Protocollo**           | Pub/Sub (Publisher/Subscriber). Ottimo per più destinatari.            | Point-to-point. Perfetto per una connessione bidirezionale.       |
| **Affidabilità**         | Garantita grazie a QoS (Quality of Service): consegna garantita a vari livelli. | Non garantisce la consegna, è un canale puro.                     |
| **Overhead**             | Basso: ottimizzato per reti a bassa larghezza di banda.                 | Medio: richiede una connessione TCP persistente.                  |
| **Gestione server**      | Necessita di un broker (es: Mosquitto).                                 | Non richiede un broker, il server WebSocket gestisce tutto.       |
| **Scalabilità**          | Elevata: il broker distribuisce i messaggi a tutti i sottoscrittori.    | Limitata dal server WebSocket.                                    |
| **Semplicità**           | Relativamente semplice con librerie come PubSubClient.                 | Richiede la gestione di connessioni persistenti.                  |
| **Caso d'uso**           | Monitoraggio continuo e affidabile (ad esempio IoT).                   | Comunicazioni interattive (chat, giochi, streaming in tempo reale).|

---

### **Quando scegliere MQTT?**
- Hai bisogno di **affidabilità**: MQTT supporta tre livelli di QoS.
- Vuoi supportare **più destinatari** senza configurazioni complesse.
- La **larghezza di banda** è una risorsa limitata.
- È accettabile una **latenza leggermente più alta** rispetto ai WebSocket.

---

### **Implementazione MQTT**

Ecco come implementare lo streaming dei dati ADC usando **MQTT** con ESP32:

#### **Codice**

```cpp
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>

// Configurazione WiFi
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Configurazione MQTT
const char* mqtt_server = "BROKER_IP";  // IP del broker MQTT (es. Mosquitto)
const int mqtt_port = 1883;            // Porta predefinita MQTT
const char* mqtt_topic = "adc/stream"; // Topic per i dati ADC

WiFiClient espClient;
PubSubClient client(espClient);

// Configurazione hardware
#define CS_PIN 5
#define DRDY_PIN 4
#define SAMPLE_RATE 30000  // Frequenza di campionamento (simulata)

// Funzioni MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentativo di connessione al broker MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Connesso!");
    } else {
      Serial.print("Connessione fallita. Stato: ");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

// Funzione per acquisizione ADC
void acquireADC() {
  SPIClass spi(VSPI);
  spi.begin(18, 19, 23, CS_PIN);
  pinMode(CS_PIN, OUTPUT);
  pinMode(DRDY_PIN, INPUT);

  while (true) {
    if (digitalRead(DRDY_PIN) == LOW) {
      // Simula lettura dati ADC
      digitalWrite(CS_PIN, LOW);
      uint8_t data[3] = {0x01, 0x02, 0x03}; // Valore simulato
      digitalWrite(CS_PIN, HIGH);
      int32_t value = (data[0] << 16) | (data[1] << 8) | data[2];
      if (value & 0x800000) value -= 0x1000000;

      // Pubblica il dato su MQTT
      char message[32];
      sprintf(message, "%ld", value);
      client.publish(mqtt_topic, message);

      // Simula frequenza di campionamento
      delayMicroseconds(1000000 / SAMPLE_RATE);
    }
  }
}

// Setup iniziale
void setup() {
  Serial.begin(115200);

  // Connessione WiFi
  Serial.print("Connessione a WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connesso!");

  // Configurazione MQTT
  client.setServer(mqtt_server, mqtt_port);

  // Inizializzazione SPI
  pinMode(CS_PIN, OUTPUT);
  pinMode(DRDY_PIN, INPUT);
}

// Loop principale
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Avvio acquisizione dati ADC
  acquireADC();
}
```

---

### **Configurazione del broker MQTT**
1. Installa un broker MQTT come **Mosquitto** su un server locale o cloud.
2. Configura il broker per accettare connessioni dal dispositivo ESP32.

---

### **Vantaggi di MQTT per il tuo progetto**
- **Robustezza**: Può gestire interruzioni di connessione grazie a QoS.
- **Facilità di scalabilità**: Altri dispositivi possono ricevere i dati sottoscrivendosi al topic.
- **Adattabilità**: Funziona bene su reti con larghezza di banda limitata.

## **MQTT a blocchi**

Pubblicare un dato a **30.000 campioni al secondo (SPS)** usando MQTT è tecnicamente possibile, ma presenta sfide significative legate a larghezza di banda, prestazioni dell'ESP32, e limitazioni intrinseche del protocollo MQTT. Vediamo i dettagli:

---

### **Problemi e Limitazioni**
1. **Overhead del Protocollo**:
   - MQTT aggiunge overhead ai messaggi, che include topic, payload, QoS, e header TCP/IP.
   - Per 30.000 messaggi al secondo, l'overhead diventa significativo.

2. **Limiti di Larghezza di Banda**:
   - Con un payload di 4 byte (ad esempio un valore ADC 32-bit) e overhead di circa 20-30 byte per messaggio:
     - **(30 + 4) * 30.000 ≈ 1 Mbps**.
   - Anche su una rete stabile, il throughput può diventare un collo di bottiglia.

3. **Prestazioni dell'ESP32**:
   - L'ESP32 potrebbe non gestire contemporaneamente l'acquisizione a 30 kSPS e l'invio MQTT senza introdurre **jitter** o perdite di dati.

4. **Broker MQTT**:
   - I broker MQTT devono gestire 30.000 messaggi al secondo per singolo client, il che potrebbe sovraccaricarli.

---

### **Possibili Soluzioni**
Per gestire un flusso di dati a 30 kSPS con MQTT, puoi adottare strategie per ridurre il numero di messaggi senza perdere dati:

#### **1. Aggregazione di Campioni**
   - Invia blocchi di dati anziché campioni singoli.
   - Ad esempio, invia blocchi di 300 campioni (10 ms di dati a 30 kSPS).
   - Riduce il numero di messaggi da 30.000 a 100 al secondo.

   ```cpp
   #define BLOCK_SIZE 300
   int32_t sample_buffer[BLOCK_SIZE];
   int buffer_index = 0;

   void acquireADC() {
       while (true) {
           if (digitalRead(DRDY_PIN) == LOW) {
               int32_t value = readADC();  // Funzione che legge l'ADC
               sample_buffer[buffer_index++] = value;

               if (buffer_index >= BLOCK_SIZE) {
                   char message[BLOCK_SIZE * 4 + 1];  // 4 byte per campione
                   memcpy(message, sample_buffer, BLOCK_SIZE * sizeof(int32_t));
                   client.publish(mqtt_topic, message, BLOCK_SIZE * sizeof(int32_t));
                   buffer_index = 0;
               }
           }
       }
   }
   ```

#### **2. Compressione dei Dati**
   - Applica una compressione per ridurre la dimensione dei dati prima dell'invio.
   - Ad esempio, usa **Delta Encoding** (invia solo la differenza tra campioni consecutivi).

#### **3. Usare QoS 0**
   - Per minimizzare il ritardo, usa **QoS 0** (fire-and-forget). Questo riduce il carico sul broker e sull'ESP32.

#### **4. Ottimizzare il Broker**
   - Usa un broker MQTT ottimizzato per alte prestazioni, come **EMQX** o **HiveMQ**, su un server con sufficiente potenza di calcolo.

#### **5. Cambiare Protocollo**
   - Se la latenza e l'affidabilità sono critiche, potresti preferire **UDP** o **WebSocket** rispetto a MQTT.

---

### **Conclusione**
Pubblicare 30.000 SPS con MQTT è **tecnicamente fattibile**, ma non è ideale. Aggregare i campioni in blocchi è il miglior compromesso, riducendo drasticamente il carico senza compromettere l'integrità dei dati.

Se il tuo progetto richiede streaming ad altissima frequenza, considera i seguenti approcci:
- MQTT con **blocchi aggregati** per affidabilità.
- **WebSocket** per flussi in tempo reale con bassa latenza.
- **UDP grezzo** per massimo throughput a scapito della consegna garantita. 

## **Confronto MQTT WebSocket**

La scelta tra **MQTT** e **WebSocket** a blocchi per quanto riguarda l'impegno della CPU dipende dalle specifiche implementazioni e dal contesto. Tuttavia, in generale possiamo fare le seguenti osservazioni:

---

### **MQTT**
#### **Caratteristiche di Impegno della CPU**
1. **Protocolli di Livello Superiore**:
   - MQTT ha un overhead maggiore rispetto ai WebSocket perché include funzionalità come gestione delle connessioni, QoS (Quality of Service), e la gestione di topic.
   - Questo si traduce in un uso più intenso della CPU per preparare e gestire i messaggi.

2. **Parsing e Encoding**:
   - MQTT utilizza un formato binario per i suoi messaggi, che richiede parsing e encoding, aumentando l'impegno computazionale.

3. **Affidabilità Incorporata**:
   - La gestione di QoS 1 o 2 implica conferme di consegna, ritrasmissioni in caso di fallimento, e sincronizzazione tra client e broker.

4. **Efficienza del Trasferimento Dati**:
   - È progettato per reti limitate in larghezza di banda, il che lo rende efficiente in termini di dati ma potenzialmente più impegnativo in termini di CPU.

---

### **WebSocket**
#### **Caratteristiche di Impegno della CPU**
1. **Protocollo più Leggero**:
   - WebSocket è un protocollo di livello più basso rispetto a MQTT e ha meno overhead. Si limita a gestire una connessione full-duplex.
   - Una volta stabilita la connessione, i dati vengono inviati come puro payload binario o testo, con overhead minimo.

2. **Nessun QoS Incorporato**:
   - Non ha la complessità del QoS, quindi non ci sono ritrasmissioni o conferme implicite che aumentino il carico della CPU.

3. **Elaborazione Diretta**:
   - I dati vengono inviati e ricevuti senza ulteriori trasformazioni o gestione avanzata dei topic.

4. **Efficienza su Larghezza di Banda**:
   - È meno ottimizzato per reti con larghezza di banda limitata, ma per connessioni stabili su reti locali o Internet ad alta velocità, il carico è generalmente inferiore.

---

### **Confronto Diretto**
| Caratteristica                  | MQTT                                  | WebSocket                            |
|---------------------------------|---------------------------------------|--------------------------------------|
| **Overhead Protocollo**          | Alto (topic, QoS, parsing)            | Basso (solo framing base)            |
| **Gestione della Connessione**   | Automatica tramite il broker          | Più leggera, client-server diretto   |
| **CPU Necessaria per Payload**   | Maggiore per parsing e QoS            | Minore, semplice invio/ricezione     |
| **Affidabilità Incorporata**     | Elevata con QoS 1 o 2                 | Deve essere implementata a parte     |
| **Efficienza CPU per Blocco**    | Minore per blocchi grandi             | Molto alta per blocchi grandi        |

---

### **Conclusione**
- **WebSocket** è generalmente meno impegnativo per la CPU rispetto a MQTT, specialmente per **trasmissioni di dati ad alta velocità e basso overhead**.
- **MQTT** offre più funzionalità integrate (QoS, topic, broker), ma richiede più CPU per gestire questi aspetti.

#### **Raccomandazione**
Se il tuo obiettivo principale è **minimizzare il carico della CPU**:
- Usa **WebSocket**, specialmente se puoi inviare blocchi di dati e non hai bisogno delle funzionalità di QoS di MQTT.
  
Se invece hai bisogno di **affidabilità integrata** o vuoi usare un sistema scalabile con un broker:
- Usa **MQTT**, ma considera l'aggregazione dei dati in blocchi per ridurre il numero di messaggi.

## **Gestione ibrida in Python**

Ecco una possibile implementazione in cui **WebSocket** e **MQTT** condividono lo stesso thread. Questo approccio sfrutta un **event loop** per gestire entrambe le comunicazioni in modo cooperativo, utilizzando librerie asincrone per WebSocket e MQTT.

---

### **Codice ESP32**
```python
import uasyncio as asyncio
from machine import Pin, SPI
from umqtt.simple import MQTTClient
import uwebsocket as websocket

# Configurazione hardware
CS_PIN = 5
DRDY_PIN = 4
TRIGGER_PIN = 16

# Parametri MQTT
MQTT_BROKER = "broker.hivemq.com"  # Sostituisci con il tuo broker MQTT
MQTT_PORT = 1883
MQTT_CLIENT_ID = "ESP32_ADC"
MQTT_TOPIC_CONFIG = b"esp32/adc/config"
MQTT_TOPIC_STATUS = b"esp32/adc/status"

# Parametri WebSocket
WEBSOCKET_URL = "ws://192.168.1.100:8080"  # Sostituisci con il server WebSocket

# Buffer circolare
BUFFER_SIZE = 1024
adc_buffer = []

# Flag globali
is_running = True  # Indica se l'acquisizione è attiva
sample_rate = 30000  # Frequenza di campionamento predefinita


async def mqtt_task():
    """Task MQTT per gestire configurazione e stato."""
    def on_message(topic, msg):
        global sample_rate, is_running
        print(f"[MQTT] Messaggio ricevuto su {topic}: {msg}")
        if topic == MQTT_TOPIC_CONFIG:
            try:
                config = msg.decode("utf-8").split("=")
                if config[0] == "sample_rate":
                    sample_rate = int(config[1])
                    print(f"Frequenza di campionamento aggiornata: {sample_rate}")
            except Exception as e:
                print(f"Errore nel parsing della configurazione: {e}")

    # Connessione al broker MQTT
    client = MQTTClient(MQTT_CLIENT_ID, MQTT_BROKER, port=MQTT_PORT)
    client.set_callback(on_message)
    client.connect()
    client.subscribe(MQTT_TOPIC_CONFIG)

    print("[MQTT] Connesso al broker")

    while is_running:
        client.check_msg()  # Verifica messaggi MQTT
        await asyncio.sleep(0.1)

    client.disconnect()
    print("[MQTT] Disconnesso")


async def websocket_task():
    """Task WebSocket per inviare dati ADC."""
    global is_running
    ws = websocket.websocket()
    await ws.connect(WEBSOCKET_URL)
    print("[WebSocket] Connessione aperta")

    try:
        while is_running:
            if adc_buffer:
                # Estrai un blocco di dati dal buffer
                data = []
                while adc_buffer and len(data) < 100:  # Invia 100 campioni per volta
                    data.append(adc_buffer.pop(0))
                await ws.send(str(data))
                print(f"[WebSocket] Inviati {len(data)} campioni")

            await asyncio.sleep(0.01)  # Attendi un breve intervallo
    except Exception as e:
        print(f"[WebSocket] Errore: {e}")
    finally:
        await ws.close()
        print("[WebSocket] Connessione chiusa")


async def adc_task():
    """Task per la lettura dei dati ADC."""
    global is_running
    # Simulazione acquisizione ADC
    while is_running:
        if len(adc_buffer) < BUFFER_SIZE:
            adc_buffer.append(42)  # Aggiungi un valore simulato al buffer
        else:
            print("[ADC] Buffer pieno, in attesa...")
        await asyncio.sleep(1 / sample_rate)


async def main():
    """Main loop."""
    print("Inizializzazione...")
    task_mqtt = asyncio.create_task(mqtt_task())
    task_websocket = asyncio.create_task(websocket_task())
    task_adc = asyncio.create_task(adc_task())

    try:
        await asyncio.gather(task_mqtt, task_websocket, task_adc)
    except KeyboardInterrupt:
        print("Terminazione forzata")
    finally:
        global is_running
        is_running = False


# Avvio
asyncio.run(main())
```

---

### **Descrizione Funzionalità**
1. **MQTT e WebSocket nello stesso thread**
   - Il task MQTT utilizza il metodo `check_msg()` per elaborare i messaggi in modo non bloccante.
   - Il task WebSocket utilizza l'API asincrona per inviare i dati ADC prelevati dal buffer circolare.

2. **Buffer Circolare**
   - Il buffer circolare (`adc_buffer`) è utilizzato come tampone per gestire il flusso di dati ADC.
   - Il task ADC popola il buffer con nuovi campioni (simulati in questo esempio).

3. **Controllo dei Parametri**
   - I comandi MQTT consentono di modificare parametri di configurazione come `sample_rate`.

4. **Gestione del Carico**
   - Entrambi i protocolli condividono lo stesso loop, ma il codice è strutturato per evitare blocchi prolungati.

---

### **Considerazioni**
- Questo approccio è semplice e funzionale per carichi moderati. Tuttavia, se il carico aumenta (ad esempio, con un flusso dati molto elevato), potrebbe essere più efficiente separare MQTT e WebSocket su thread/core differenti.
- Puoi testare e ottimizzare i tempi di `await` per bilanciare il carico tra le varie operazioni.

>[Torna all'indice](readme.md#fasi-progetto)
