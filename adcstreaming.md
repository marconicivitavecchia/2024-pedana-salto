>[Torna all'indice](readme.md#fasi-progetto)


### **Condizioni di Bernstein**

Le condizioni si basano sui concetti di insiemi di dati **letti** (read set) e **scritti** (write set) da ciascun processo:

- **Read set (R)**: insieme dei dati che il processo legge.
- **Write set (W)**: insieme dei dati che il processo scrive.

Per due processi $$P_1$$ e $$P_2$$, non ci sono interferenze (quindi possono essere eseguiti in parallelo) se sono soddisfatte le seguenti condizioni:

1. $$W_1 \cap W_2 = \emptyset$$: i processi non scrivono sugli stessi dati.
2. $$W_1 \cap R_2 = \emptyset$$: il primo processo non scrive dati che il secondo legge.
3. $$R_1 \cap W_2 = \emptyset$$: il secondo processo non scrive dati che il primo legge.

In altre parole:
- I dati scritti da un processo non devono essere letti o scritti dall'altro.
- I dati letti da un processo non devono essere scritti dall'altro.

---

### **Esempio Pratico**
Immagina di avere due processi $$P_1$$ e $$P_2$$:
- $$P_1 $$: legge $$A$$ e scrive $$B$$.
- $$P_2$$: legge $$B$$ e scrive $$C$$.

- $$R_1 = \{ A \}, W_1 = \{ B \}$$
- $$R_2 = \{ B \}, W_2 = \{ C \}$$

Verifica delle condizioni:
1. $$W_1 \cap W_2 = \emptyset$$: $$\{ B \} \cap \{ C \} = \emptyset$$ ✅
2. $$W_1 \cap R_2 = \emptyset$$: $$\{ B \} \cap \{ B \} \neq \emptyset$$ ❌
3. $$R_1 \cap W_2 = \emptyset$$: $$\{ A \} \cap \{ C \} = \emptyset$$ ✅

In questo caso, i due processi **non possono essere eseguiti in parallelo** perché $W_1 \cap R_2 \neq \emptyset$: il primo processo scrive $$B$$, che il secondo processo legge.

---

### **Applicazione in Sistemi Concreti**
Le condizioni di Bernstein sono utili per:
- Verificare la possibilità di **parallelizzare task** senza introdurre condizioni di corsa.
- Progettare sistemi in cui i dati condivisi sono protetti tramite tecniche come **mutex**, **sezioni critiche** o **variabili atomiche**.

Nel tuo caso con il buffer circolare:
- L'ISR scrive i dati nel buffer.
- Il task WebSocket legge i dati dal buffer.
  
Verifichiamo:
- L'ISR scrive $$W_{\text{ISR}} = \{\text{buffer}, \text{writeIndex}, \text{bufferCount}\}$$.
- Il task WebSocket legge \$$R_{\text{WebSocket}} = \{\text{buffer}, \text{readIndex}, \text{bufferCount}\}$$ e scrive $$W_{\text{WebSocket}} = \{\text{readIndex}, \text{bufferCount}\}$$.

In questo caso, $$W_{\text{ISR}} \cap R_{\text{WebSocket}} = \{\text{bufferCount}\}$$, quindi c'è un'interferenza, e serve una protezione (ad esempio un **mutex** o sezioni critiche).

---

Ecco una tabella che riassume chiaramente le interferenze tra i due task basata sulle **condizioni di Bernstein**. Ho incluso riferimenti alla teoria per chiarezza.

---

### **Tabella delle Interferenze (Task ISR e WebSocket)**

| **Dati condivisi** | **Accesso ISR**      | **Accesso WebSocket** | **Interferenza** | **Violazione Bernstein**              |
|---------------------|----------------------|------------------------|-------------------|---------------------------------------|
| `buffer`           | **Scrittura**       | **Lettura**            | **Sì**           | $$W_{\text{ISR}} \cap R_{\text{WebSocket}} \neq \emptyset$$ |
| `writeIndex`       | **Scrittura**       | **Nessun accesso**     | **No**           | Nessuna interferenza                  |
| `readIndex`        | **Nessun accesso**  | **Scrittura**          | **No**           | Nessuna interferenza                  |
| `bufferCount`      | **Scrittura**       | **Lettura e Scrittura**| **Sì**           | $$W_{\text{ISR}} \cap (R_{\text{WebSocket}} \cup W_{\text{WebSocket}}) \neq \emptyset$$ |

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
1. **buffer**: $$W_{\text{ISR}} \cap R_{\text{WebSocket}} \neq \emptyset$$.
2. **bufferCount**: $$W_{\text{ISR}} \cap (R_{\text{WebSocket}} \cup W_{\text{WebSocket}}) \neq \emptyset$$.

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

## **Versione ibrida in C++**

```C++
#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SPI.h>

// Configurazione hardware
#define CS_PIN 5
#define DRDY_PIN 4

// Configurazione acquisizione
#define QUEUE_SIZE 1024
#define DEFAULT_SAMPLE_RATE 30000  // Hz

// Struttura configurazione
struct Config {
    uint32_t sampleRate;
    uint8_t gain;
    bool filterEnabled;
    float threshold;
    bool streaming;
};

// Variabili globali
QueueHandle_t dataQueue;
QueueHandle_t eventQueue;
SemaphoreHandle_t configMutex;
volatile Config config;
volatile bool shouldRestart = false;

// Configurazione rete
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";
const char* MQTT_SERVER = "YOUR_MQTT_SERVER";
const int MQTT_PORT = 1883;
const char* MQTT_USER = "YOUR_MQTT_USER";
const char* MQTT_PASSWORD = "YOUR_MQTT_PASSWORD";

// Topic MQTT
const char* MQTT_CONFIG_TOPIC = "device/config";
const char* MQTT_STATUS_TOPIC = "device/status";
const char* MQTT_EVENT_TOPIC = "device/events";

// Oggetti per comunicazione
WebSocketsServer webSocket(81);
WiFiClient espClient;
PubSubClient mqtt(espClient);

// Struttura per eventi
struct Event {
    uint32_t timestamp;
    char type[32];
    float value;
};

// Callback WebSocket
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    switch(type) {
        case WStype_CONNECTED:
            Serial.printf("[WebSocket] Client #%u connesso\n", num);
            break;
            
        case WStype_DISCONNECTED:
            Serial.printf("[WebSocket] Client #%u disconnesso\n", num);
            break;
            
        case WStype_TEXT:
            // Gestione comandi WebSocket (se necessario)
            break;
    }
}

// Callback MQTT
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    
    if (error) {
        Serial.println("Errore parsing JSON");
        return;
    }
    
    if (strcmp(topic, MQTT_CONFIG_TOPIC) == 0) {
        xSemaphoreTake(configMutex, portMAX_DELAY);
        
        if (doc.containsKey("sampleRate")) 
            config.sampleRate = doc["sampleRate"];
        if (doc.containsKey("gain"))
            config.gain = doc["gain"];
        if (doc.containsKey("filterEnabled"))
            config.filterEnabled = doc["filterEnabled"];
        if (doc.containsKey("threshold"))
            config.threshold = doc["threshold"];
        if (doc.containsKey("streaming"))
            config.streaming = doc["streaming"];
            
        xSemaphoreGive(configMutex);
        
        // Segnala necessità restart acquisizione
        shouldRestart = true;
        
        // Pubblica conferma
        publishStatus();
    }
}

// Pubblica stato corrente
void publishStatus() {
    StaticJsonDocument<512> doc;
    
    xSemaphoreTake(configMutex, portMAX_DELAY);
    doc["sampleRate"] = config.sampleRate;
    doc["gain"] = config.gain;
    doc["filterEnabled"] = config.filterEnabled;
    doc["threshold"] = config.threshold;
    doc["streaming"] = config.streaming;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["uptime"] = millis() / 1000;
    xSemaphoreGive(configMutex);
    
    char buffer[512];
    serializeJson(doc, buffer);
    mqtt.publish(MQTT_STATUS_TOPIC, buffer);
}

// Task acquisizione ADC
void adcTask(void* pvParameters) {
    SPIClass spi(VSPI);
    spi.begin(18, 19, 23, CS_PIN);
    pinMode(CS_PIN, OUTPUT);
    pinMode(DRDY_PIN, INPUT);
    
    uint32_t lastSample = 0;
    uint32_t sampleInterval;
    
    while (true) {
        xSemaphoreTake(configMutex, portMAX_DELAY);
        if (!config.streaming) {
            xSemaphoreGive(configMutex);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }
        sampleInterval = 1000000 / config.sampleRate;
        xSemaphoreGive(configMutex);
        
        if (digitalRead(DRDY_PIN) == LOW && 
            (micros() - lastSample) >= sampleInterval) {
            
            // Lettura ADC
            digitalWrite(CS_PIN, LOW);
            uint8_t data[3] = {0x01, 0x02, 0x03}; // Simulato
            digitalWrite(CS_PIN, HIGH);
            
            int32_t value = (data[0] << 16) | (data[1] << 8) | data[2];
            if (value & 0x800000) value -= 0x1000000;
            
            // Applica filtro se abilitato
            xSemaphoreTake(configMutex, portMAX_DELAY);
            if (config.filterEnabled) {
                // Implementa qui il filtro
            }
            
            // Verifica threshold
            if (abs(value) > config.threshold) {
                Event evt = {
                    .timestamp = millis(),
                    .value = value
                };
                strcpy(evt.type, "threshold_exceeded");
                xQueueSend(eventQueue, &evt, 0);
            }
            xSemaphoreGive(configMutex);
            
            // Invia alla coda
            if (xQueueSend(dataQueue, &value, 0) != pdTRUE) {
                // Coda piena, gestione overflow
                Event evt = {
                    .timestamp = millis(),
                    .value = 0
                };
                strcpy(evt.type, "queue_overflow");
                xQueueSend(eventQueue, &evt, 0);
            }
            
            lastSample = micros();
        }
        
        // Controlla flag restart
        if (shouldRestart) {
            shouldRestart = false;
            lastSample = 0;
            // Pulizia code
            xQueueReset(dataQueue);
        }
    }
}

// Task streaming WebSocket
void webSocketTask(void* pvParameters) {
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    
    while (true) {
        webSocket.loop();
        
        int32_t value;
        if (xQueueReceive(dataQueue, &value, 0) == pdTRUE) {
            // Invia solo se ci sono client connessi
            if (webSocket.connectedClients() > 0) {
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%ld", value);
                webSocket.broadcastTXT(buffer);
            }
        } else {
            // Nessun dato disponibile, yield
            vTaskDelay(1);
        }
    }
}

// Task gestione eventi
void eventTask(void* pvParameters) {
    while (true) {
        Event evt;
        if (xQueueReceive(eventQueue, &evt, 1000 / portTICK_PERIOD_MS) == pdTRUE) {
            StaticJsonDocument<256> doc;
            doc["timestamp"] = evt.timestamp;
            doc["type"] = evt.type;
            doc["value"] = evt.value;
            
            char buffer[256];
            serializeJson(doc, buffer);
            mqtt.publish(MQTT_EVENT_TOPIC, buffer);
        }
        
        // Pubblica stato periodicamente
        static uint32_t lastStatus = 0;
        if (millis() - lastStatus > 10000) {  // Ogni 10 secondi
            publishStatus();
            lastStatus = millis();
        }
    }
}

// Task reconnessione MQTT
void mqttReconnectTask(void* pvParameters) {
    while (true) {
        if (!mqtt.connected()) {
            Serial.println("Tentativo connessione MQTT...");
            if (mqtt.connect("ESP32Client", MQTT_USER, MQTT_PASSWORD)) {
                Serial.println("Connesso a MQTT");
                mqtt.subscribe(MQTT_CONFIG_TOPIC);
                publishStatus();
            }
        }
        mqtt.loop();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void setup() {
    Serial.begin(115200);
    
    // Inizializza strutture
    dataQueue = xQueueCreate(QUEUE_SIZE, sizeof(int32_t));
    eventQueue = xQueueCreate(32, sizeof(Event));
    configMutex = xSemaphoreCreateMutex();
    
    // Configurazione default
    config.sampleRate = DEFAULT_SAMPLE_RATE;
    config.gain = 1;
    config.filterEnabled = false;
    config.threshold = 1000000;
    config.streaming = true;
    
    // Connessione WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnesso al WiFi");
    
    // Setup MQTT
    mqtt.setServer(MQTT_SERVER, MQTT_PORT);
    mqtt.setCallback(mqttCallback);
    
    // Creazione task
    xTaskCreatePinnedToCore(adcTask, "ADC", 8192, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(webSocketTask, "WebSocket", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(eventTask, "Events", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(mqttReconnectTask, "MQTT", 4096, NULL, 1, NULL, 1);
}

void loop() {
    vTaskDelete(NULL);  // Non necessario nel loop principale
}
```

I semafori (in particolare il mutex nel codice) e le code servono a scopi diversi:

Le code (QueueHandle_t) si usano per passare dati tra task in modo thread-safe, come nel caso dei campioni ADC che passano dal task di acquisizione al task WebSocket. È corretto che le code già garantiscono la sincronizzazione per i dati che vengono scambiati.

Il mutex (configMutex) invece serve a proteggere l'accesso alla struttura `config` che è una variabile globale condivisa tra più task:

```cpp
struct Config {
    uint32_t sampleRate;
    uint8_t gain;
    bool filterEnabled;
    float threshold;
    bool streaming;
};
volatile Config config;
```

Questa struttura viene:
1. Letta dal task ADC per sapere frequenza e parametri di acquisizione
2. Modificata dal callback MQTT quando arriva un comando di configurazione
3. Letta dal task eventi per pubblicare lo stato

Senza il mutex, potrebbero verificarsi race condition, per esempio:
- Il task ADC sta leggendo sampleRate mentre il callback MQTT lo sta modificando
- Il task eventi sta leggendo i parametri per pubblicarli mentre vengono cambiati

Con il mutex invece:

```cpp
// Task ADC legge configurazione
xSemaphoreTake(configMutex, portMAX_DELAY);
sampleInterval = 1000000 / config.sampleRate;
xSemaphoreGive(configMutex);

// Callback MQTT modifica configurazione
xSemaphoreTake(configMutex, portMAX_DELAY);
config.sampleRate = doc["sampleRate"];
xSemaphoreGive(configMutex);
```

Si potrebbe evitare il mutex:
1. Usando solo code anche per la configurazione
2. Copiando localmente i parametri necessari all'avvio di ogni task
3. Usando strutture dati lock-free 

## **Copia locale i parametri all'avvio di ogni task**

Ecco una versione riscritta che evita il mutex copiando localmente i parametri nei task. Questo approccio è più pulito e riduce la contesa tra i task.



```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SPI.h>

// Configurazione hardware
#define CS_PIN 5
#define DRDY_PIN 4

// Configurazione acquisizione
#define QUEUE_SIZE 1024
#define DEFAULT_SAMPLE_RATE 30000  // Hz

// Struttura configurazione
struct Config {
    uint32_t sampleRate;
    uint8_t gain;
    bool filterEnabled;
    float threshold;
    bool streaming;
};

// Struttura comando configurazione
struct ConfigCommand {
    enum Type {
        SAMPLE_RATE,
        GAIN,
        FILTER,
        THRESHOLD,
        STREAMING
    };
    Type type;
    float value;
};

// Struttura evento
struct Event {
    uint32_t timestamp;
    char type[32];
    float value;
};

// Code per comunicazione tra task
QueueHandle_t dataQueue;        // Campioni ADC
QueueHandle_t eventQueue;       // Eventi da pubblicare
QueueHandle_t configQueue;      // Comandi configurazione

// Configurazione rete
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";
const char* MQTT_SERVER = "YOUR_MQTT_SERVER";
const int MQTT_PORT = 1883;
const char* MQTT_USER = "YOUR_MQTT_USER";
const char* MQTT_PASSWORD = "YOUR_MQTT_PASSWORD";

// Topic MQTT
const char* MQTT_CONFIG_TOPIC = "device/config";
const char* MQTT_STATUS_TOPIC = "device/status";
const char* MQTT_EVENT_TOPIC = "device/events";

// Oggetti per comunicazione
WebSocketsServer webSocket(81);
WiFiClient espClient;
PubSubClient mqtt(espClient);

// Callback MQTT - Invia comandi di configurazione tramite coda
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    
    if (error) {
        Serial.println("Errore parsing JSON");
        return;
    }
    
    if (strcmp(topic, MQTT_CONFIG_TOPIC) == 0) {
        ConfigCommand cmd;
        
        // Converte il JSON in comandi individuali
        if (doc.containsKey("sampleRate")) {
            cmd.type = ConfigCommand::SAMPLE_RATE;
            cmd.value = doc["sampleRate"];
            xQueueSend(configQueue, &cmd, 0);
        }
        if (doc.containsKey("gain")) {
            cmd.type = ConfigCommand::GAIN;
            cmd.value = doc["gain"];
            xQueueSend(configQueue, &cmd, 0);
        }
        if (doc.containsKey("filterEnabled")) {
            cmd.type = ConfigCommand::FILTER;
            cmd.value = doc["filterEnabled"];
            xQueueSend(configQueue, &cmd, 0);
        }
        if (doc.containsKey("threshold")) {
            cmd.type = ConfigCommand::THRESHOLD;
            cmd.value = doc["threshold"];
            xQueueSend(configQueue, &cmd, 0);
        }
        if (doc.containsKey("streaming")) {
            cmd.type = ConfigCommand::STREAMING;
            cmd.value = doc["streaming"];
            xQueueSend(configQueue, &cmd, 0);
        }
    }
}

// Task acquisizione ADC
void adcTask(void* pvParameters) {
    // Configurazione locale
    Config localConfig = {
        .sampleRate = DEFAULT_SAMPLE_RATE,
        .gain = 1,
        .filterEnabled = false,
        .threshold = 1000000,
        .streaming = true
    };
    
    SPIClass spi(VSPI);
    spi.begin(18, 19, 23, CS_PIN);
    pinMode(CS_PIN, OUTPUT);
    pinMode(DRDY_PIN, INPUT);
    
    uint32_t lastSample = 0;
    uint32_t sampleInterval = 1000000 / localConfig.sampleRate;
    
    while (true) {
        // Controlla aggiornamenti configurazione
        ConfigCommand cmd;
        while (xQueueReceive(configQueue, &cmd, 0) == pdTRUE) {
            switch (cmd.type) {
                case ConfigCommand::SAMPLE_RATE:
                    localConfig.sampleRate = cmd.value;
                    sampleInterval = 1000000 / localConfig.sampleRate;
                    break;
                case ConfigCommand::GAIN:
                    localConfig.gain = cmd.value;
                    break;
                case ConfigCommand::FILTER:
                    localConfig.filterEnabled = cmd.value;
                    break;
                case ConfigCommand::THRESHOLD:
                    localConfig.threshold = cmd.value;
                    break;
                case ConfigCommand::STREAMING:
                    localConfig.streaming = cmd.value;
                    break;
            }
        }
        
        if (!localConfig.streaming) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }
        
        if (digitalRead(DRDY_PIN) == LOW && 
            (micros() - lastSample) >= sampleInterval) {
            
            // Lettura ADC (simulata)
            digitalWrite(CS_PIN, LOW);
            uint8_t data[3] = {0x01, 0x02, 0x03};
            digitalWrite(CS_PIN, HIGH);
            
            int32_t value = (data[0] << 16) | (data[1] << 8) | data[2];
            if (value & 0x800000) value -= 0x1000000;
            
            // Applica gain
            value *= localConfig.gain;
            
            // Applica filtro se abilitato
            if (localConfig.filterEnabled) {
                // Implementa qui il filtro
            }
            
            // Verifica threshold
            if (abs(value) > localConfig.threshold) {
                Event evt = {
                    .timestamp = millis(),
                    .value = value
                };
                strcpy(evt.type, "threshold_exceeded");
                xQueueSend(eventQueue, &evt, 0);
            }
            
            // Invia alla coda dati
            if (xQueueSend(dataQueue, &value, 0) != pdTRUE) {
                Event evt = {
                    .timestamp = millis(),
                    .value = 0
                };
                strcpy(evt.type, "queue_overflow");
                xQueueSend(eventQueue, &evt, 0);
            }
            
            lastSample = micros();
        }
    }
}

// Task streaming WebSocket
void webSocketTask(void* pvParameters) {
    webSocket.begin();
    webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
        switch(type) {
            case WStype_CONNECTED:
                Serial.printf("[WebSocket] Client #%u connesso\n", num);
                break;
            case WStype_DISCONNECTED:
                Serial.printf("[WebSocket] Client #%u disconnesso\n", num);
                break;
        }
    });
    
    while (true) {
        webSocket.loop();
        
        int32_t value;
        if (xQueueReceive(dataQueue, &value, 0) == pdTRUE) {
            if (webSocket.connectedClients() > 0) {
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%ld", value);
                webSocket.broadcastTXT(buffer);
            }
        } else {
            vTaskDelay(1);
        }
    }
}

// Task gestione eventi
void eventTask(void* pvParameters) {
    // Configurazione locale per status
    Config localConfig = {
        .sampleRate = DEFAULT_SAMPLE_RATE,
        .gain = 1,
        .filterEnabled = false,
        .threshold = 1000000,
        .streaming = true
    };
    
    while (true) {
        // Aggiorna configurazione locale
        ConfigCommand cmd;
        while (xQueueReceive(configQueue, &cmd, 0) == pdTRUE) {
            switch (cmd.type) {
                case ConfigCommand::SAMPLE_RATE:
                    localConfig.sampleRate = cmd.value;
                    break;
                case ConfigCommand::GAIN:
                    localConfig.gain = cmd.value;
                    break;
                case ConfigCommand::FILTER:
                    localConfig.filterEnabled = cmd.value;
                    break;
                case ConfigCommand::THRESHOLD:
                    localConfig.threshold = cmd.value;
                    break;
                case ConfigCommand::STREAMING:
                    localConfig.streaming = cmd.value;
                    break;
            }
        }
        
        // Gestione eventi
        Event evt;
        if (xQueueReceive(eventQueue, &evt, 1000 / portTICK_PERIOD_MS) == pdTRUE) {
            StaticJsonDocument<256> doc;
            doc["timestamp"] = evt.timestamp;
            doc["type"] = evt.type;
            doc["value"] = evt.value;
            
            char buffer[256];
            serializeJson(doc, buffer);
            mqtt.publish(MQTT_EVENT_TOPIC, buffer);
        }
        
        // Status periodico
        static uint32_t lastStatus = 0;
        if (millis() - lastStatus > 10000) {
            StaticJsonDocument<512> doc;
            doc["sampleRate"] = localConfig.sampleRate;
            doc["gain"] = localConfig.gain;
            doc["filterEnabled"] = localConfig.filterEnabled;
            doc["threshold"] = localConfig.threshold;
            doc["streaming"] = localConfig.streaming;
            doc["freeHeap"] = ESP.getFreeHeap();
            doc["uptime"] = millis() / 1000;
            
            char buffer[512];
            serializeJson(doc, buffer);
            mqtt.publish(MQTT_STATUS_TOPIC, buffer);
            
            lastStatus = millis();
        }
    }
}

void setup() {
    Serial.begin(115200);
    
    // Inizializza code
    dataQueue = xQueueCreate(QUEUE_SIZE, sizeof(int32_t));
    eventQueue = xQueueCreate(32, sizeof(Event));
    configQueue = xQueueCreate(32, sizeof(ConfigCommand));
    
    // Setup WiFi e MQTT
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnesso al WiFi");
    
    mqtt.setServer(MQTT_SERVER, MQTT_PORT);
    mqtt.setCallback(mqttCallback);
    
    // Avvia task
    xTaskCreatePinnedToCore(adcTask, "ADC", 8192, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(webSocketTask, "WebSocket", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(eventTask, "Events", 4096, NULL, 1, NULL, 1);
}

void loop() {
    // Gestione connessione MQTT nel loop principale
    if (!mqtt.connected()) {
        if (mqtt.connect("ESP32Client", MQTT_USER, MQTT_PASSWORD)) {
            mqtt.subscribe(MQTT_CONFIG_TOPIC);
        }
        delay(5000);
    }
    mqtt.loop();
}

```

I principali cambiamenti sono:

1. Rimozione del mutex e della configurazione globale
2. Ogni task mantiene una copia locale della configurazione
3. Introduzione di una struttura ConfigCommand per inviare comandi di configurazione tramite coda
4. Il callback MQTT converte il JSON in comandi singoli
5. I task aggiornano la loro configurazione locale quando ricevono comandi

Vantaggi di questo approccio:
- Non c'è contesa su variabili condivise
- Ogni task ha accesso immediato ai suoi parametri
- Il sistema è più robusto e predicibile
- Minor overhead di sincronizzazione

L'unico "costo" è che dobbiamo distribuire gli aggiornamenti di configurazione a tutti i task interessati, ma questo è gestito in modo efficiente attraverso la coda di configurazione.

## **Copia dei dati vs coda di messaggi**

L'opzione 2 (copia locale della configurazione in ogni task) rispetto alle code ha questi aspetti:

Vantaggi:
1. Accesso immediato ai parametri di configurazione senza ritardi da code
2. Minor overhead perché non serve continuamente leggere/scrivere dalla coda
3. Ogni task ha una visione coerente della sua configurazione
4. Meno complessità del codice (no gestione memoria dinamica o sincronizzazione)

Svantaggi:
1. Ridondanza dei dati (ogni task ha una copia)
2. Possibile inconsistenza temporanea tra le copie nei vari task
3. Spreco di RAM per le copie multiple
4. Aggiornamenti più complessi (devi propagare a tutti)

Le code FreeRTOS sono migliori quando:
- Hai dati che devono fluire costantemente tra task (come i campioni ADC)
- Serve garanzia di consegna
- I dati sono consumati una sola volta

La copia locale è migliore quando:
- I dati sono letti spesso e modificati raramente (come parametri di configurazione)
- La consistenza immediata non è critica
- Vuoi minimizzare la latenza di accesso

Nel caso dello streaming ADC, un approccio ibrido è ottimale:
- Code per i campioni ADC e gli eventi
- Copie locali per la configurazione

## **Versione con server Web Socket nativo esp32**

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include "esp_log.h"
#include "esp_websocket_server.h"
#include <cJSON.h>

// Configurazione hardware
#define CS_PIN 5
#define DRDY_PIN 4

// Configurazione acquisizione
#define DEFAULT_SAMPLE_RATE 30000 // Hz

// Configurazione Wi-Fi
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";

// Gestione configurazione
struct Config {
    uint32_t sampleRate;
    uint8_t gain;
    bool filterEnabled;
    float threshold;
    bool streaming;
};

// Variabili globali
QueueHandle_t dataQueue;
QueueHandle_t eventQueue;
httpd_handle_t wsDataServer = NULL;
httpd_handle_t wsEventServer = NULL;
Config globalConfig = {DEFAULT_SAMPLE_RATE, 1, false, 1000000, true};
float emaAlpha = 0.1; // Coefficiente EMA

// Callback WebSocket per gli eventi
static esp_err_t websocket_event_handler(httpd_req_t* req) {
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    // Ricezione messaggio
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE("WS_EVENT", "Errore ricezione frame");
        return ret;
    }

    ws_pkt.payload = (uint8_t*)malloc(ws_pkt.len + 1);
    if (ws_pkt.payload == NULL) {
        ESP_LOGE("WS_EVENT", "Memoria insufficiente");
        return ESP_ERR_NO_MEM;
    }

    ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    ws_pkt.payload[ws_pkt.len] = '\0';

    ESP_LOGI("WS_EVENT", "Messaggio ricevuto: %s", (char*)ws_pkt.payload);

    // Parsing del messaggio JSON
    cJSON* json = cJSON_Parse((char*)ws_pkt.payload);
    if (json) {
        cJSON* command = cJSON_GetObjectItem(json, "command");
        if (command && cJSON_IsString(command)) {
            if (strcmp(command->valuestring, "set_sample_rate") == 0) {
                cJSON* value = cJSON_GetObjectItem(json, "value");
                if (value && cJSON_IsNumber(value)) {
                    globalConfig.sampleRate = value->valueint;
                    ESP_LOGI("WS_EVENT", "Sample rate impostato a: %d", globalConfig.sampleRate);
                }
            } else if (strcmp(command->valuestring, "set_ema_alpha") == 0) {
                cJSON* value = cJSON_GetObjectItem(json, "value");
                if (value && cJSON_IsNumber(value)) {
                    emaAlpha = value->valuedouble;
                    ESP_LOGI("WS_EVENT", "EMA alpha impostato a: %.2f", emaAlpha);
                }
            }
        }
        cJSON_Delete(json);
    } else {
        ESP_LOGE("WS_EVENT", "Errore parsing JSON");
    }

    free(ws_pkt.payload);
    return ESP_OK;
}

// Configura il server WebSocket per gli eventi
static void start_event_websocket_server() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 82;

    if (httpd_start(&wsEventServer, &config) == ESP_OK) {
        httpd_uri_t ws_uri = {
            .uri = "/events",
            .method = HTTP_GET,
            .handler = websocket_event_handler,
            .user_ctx = NULL,
            .is_websocket = true
        };
        httpd_register_uri_handler(wsEventServer, &ws_uri);
        ESP_LOGI("WS_EVENT", "WebSocket server per gli eventi avviato su /events");
    } else {
        ESP_LOGE("WS_EVENT", "Errore avvio WebSocket server per gli eventi");
    }
}

// Task acquisizione dati ADC
void adcTask(void* pvParameters) {
    SPIClass spi(VSPI);
    spi.begin(18, 19, 23, CS_PIN);
    pinMode(CS_PIN, OUTPUT);
    pinMode(DRDY_PIN, INPUT);

    uint32_t lastSample = 0;
    uint32_t sampleInterval = 1000000 / globalConfig.sampleRate;
    float emaFilteredValue = 0.0;

    while (true) {
        if (!globalConfig.streaming) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        if (digitalRead(DRDY_PIN) == LOW &&
            (micros() - lastSample) >= sampleInterval) {

            digitalWrite(CS_PIN, LOW);
            uint8_t data[3] = {0x01, 0x02, 0x03};
            digitalWrite(CS_PIN, HIGH);

            int32_t value = (data[0] << 16) | (data[1] << 8) | data[2];
            if (value & 0x800000) value -= 0x1000000;

            emaFilteredValue = emaAlpha * value + (1 - emaAlpha) * emaFilteredValue;

            xQueueSend(dataQueue, &emaFilteredValue, 0);
            lastSample = micros();
        }
    }
}

// Altre funzioni rimangono invariate...

// Setup principale
void setup() {
    Serial.begin(115200);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnesso al WiFi");

    // Crea le code
    dataQueue = xQueueCreate(1024, sizeof(float));
    eventQueue = xQueueCreate(32, sizeof(char*));

    // Avvia task
    xTaskCreatePinnedToCore(adcTask, "ADC Task", 8192, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(webSocketDataTask, "WS Data Task", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(webSocketEventTask, "WS Event Task", 4096, NULL, 1, NULL, 1);

    // Avvia server WebSocket
    start_data_websocket_server();
    start_event_websocket_server();
}

void loop() {
    // Loop vuoto, tutto è gestito nei task
}
```

Il codice è stato aggiornato per includere:

- Gestione dei comandi JSON nel WebSocket degli eventi per impostare la velocità di campionamento (sampleRate) e il coefficiente alfa del filtro EMA (emaAlpha).
- Implementazione del filtro EMA nell'elaborazione dei dati ADC.
- Validazione e aggiornamento dinamico di sampleRate e emaAlpha tramite i comandi ricevuti.

Ecco un esempio di comando JSON che può essere inviato tramite il WebSocket degli eventi per impostare il sample rate e il coefficiente EMA alpha:

Impostazione del sample rate:
```json
{
    "command": "set_sample_rate",
    "value": 20000
}
```
Questo comando imposta il sample rate a 20.000 Hz.



## **Versione con formato comandi alternatvo**

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include "esp_log.h"
#include "esp_websocket_server.h"
#include <cJSON.h>

// Configurazione hardware
#define CS_PIN 5
#define DRDY_PIN 4

// Configurazione acquisizione
#define DEFAULT_SAMPLE_RATE 30000 // Hz

// Configurazione Wi-Fi
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";

// Gestione configurazione
struct Config {
    uint32_t sampleRate;
    uint8_t gain;
    bool filterEnabled;
    float threshold;
    bool streaming;
};

// Variabili globali
QueueHandle_t dataQueue;
QueueHandle_t eventQueue;
httpd_handle_t wsDataServer = NULL;
httpd_handle_t wsEventServer = NULL;
Config globalConfig = {DEFAULT_SAMPLE_RATE, 1, false, 1000000, true};
float emaAlpha = 0.1; // Coefficiente EMA

// Callback WebSocket per gli eventi
static esp_err_t websocket_event_handler(httpd_req_t* req) {
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    // Ricezione messaggio
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE("WS_EVENT", "Errore ricezione frame");
        return ret;
    }

    ws_pkt.payload = (uint8_t*)malloc(ws_pkt.len + 1);
    if (ws_pkt.payload == NULL) {
        ESP_LOGE("WS_EVENT", "Memoria insufficiente");
        return ESP_ERR_NO_MEM;
    }

    ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    ws_pkt.payload[ws_pkt.len] = '\0';

    ESP_LOGI("WS_EVENT", "Messaggio ricevuto: %s", (char*)ws_pkt.payload);

    // Parsing del messaggio JSON
    // Parsing del messaggio JSON
    cJSON* json = cJSON_Parse((char*)ws_pkt.payload);
    if (json) {
        cJSON* samplerate = cJSON_GetObjectItem(json, "samplerate");
        if (samplerate && cJSON_IsString(samplerate)) {
            globalConfig.sampleRate = atoi(samplerate->valuestring);
            ESP_LOGI("WS_EVENT", "Sample rate impostato a: %d", globalConfig.sampleRate);
        }
    
        cJSON* alfaema = cJSON_GetObjectItem(json, "alfaema");
        if (alfaema && cJSON_IsString(alfaema)) {
            emaAlpha = atof(alfaema->valuestring);
            ESP_LOGI("WS_EVENT", "EMA alpha impostato a: %.2f", emaAlpha);
        }
    
        cJSON_Delete(json);
    } else {
        ESP_LOGE("WS_EVENT", "Errore parsing JSON");
    }


    free(ws_pkt.payload);
    return ESP_OK;
}

// Configura il server WebSocket per gli eventi
static void start_event_websocket_server() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 82;

    if (httpd_start(&wsEventServer, &config) == ESP_OK) {
        httpd_uri_t ws_uri = {
            .uri = "/events",
            .method = HTTP_GET,
            .handler = websocket_event_handler,
            .user_ctx = NULL,
            .is_websocket = true
        };
        httpd_register_uri_handler(wsEventServer, &ws_uri);
        ESP_LOGI("WS_EVENT", "WebSocket server per gli eventi avviato su /events");
    } else {
        ESP_LOGE("WS_EVENT", "Errore avvio WebSocket server per gli eventi");
    }
}

// Task acquisizione dati ADC
void adcTask(void* pvParameters) {
    SPIClass spi(VSPI);
    spi.begin(18, 19, 23, CS_PIN);
    pinMode(CS_PIN, OUTPUT);
    pinMode(DRDY_PIN, INPUT);

    uint32_t lastSample = 0;
    uint32_t sampleInterval = 1000000 / globalConfig.sampleRate;
    float emaFilteredValue = 0.0;

    while (true) {
        if (!globalConfig.streaming) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        if (digitalRead(DRDY_PIN) == LOW &&
            (micros() - lastSample) >= sampleInterval) {

            digitalWrite(CS_PIN, LOW);
            uint8_t data[3] = {0x01, 0x02, 0x03};
            digitalWrite(CS_PIN, HIGH);

            int32_t value = (data[0] << 16) | (data[1] << 8) | data[2];
            if (value & 0x800000) value -= 0x1000000;

            emaFilteredValue = emaAlpha * value + (1 - emaAlpha) * emaFilteredValue;

            xQueueSend(dataQueue, &emaFilteredValue, 0);
            lastSample = micros();
        }
    }
}

// Altre funzioni rimangono invariate...

// Setup principale
void setup() {
    Serial.begin(115200);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnesso al WiFi");

    // Crea le code
    dataQueue = xQueueCreate(1024, sizeof(float));
    eventQueue = xQueueCreate(32, sizeof(char*));

    // Avvia task
    xTaskCreatePinnedToCore(adcTask, "ADC Task", 8192, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(webSocketDataTask, "WS Data Task", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(webSocketEventTask, "WS Event Task", 4096, NULL, 1, NULL, 1);

    // Avvia server WebSocket
    start_data_websocket_server();
    start_event_websocket_server();
}

void loop() {
    // Loop vuoto, tutto è gestito nei task
}
```

### **Esempi di comando JSON**

- Impostare solo il sample rate:

```json
{"samplerate": "10000"}
```
- Impostare solo il coefficiente EMA alpha:
```json
{"alfaema": "0.5"}
```
- Impostare entrambi:
```json
{"samplerate": "20000", "alfaema": "0.8"}
```

### **Modifica al adcTask per test**

Aggiungere questa configurazione al task ADC:

```C++
void adcTask(void* pvParameters) {
    uint32_t lastSample = 0;
    uint32_t sampleInterval = 1000000 / globalConfig.sampleRate;

    while (true) {
        if (!globalConfig.streaming) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        if ((micros() - lastSample) >= sampleInterval) {
            uint32_t timestamp = micros();
            float sample = random(0, 1000);  // Campioni casuali (da 0 a 1000)

            // Pacchetto JSON con campione e timestamp
            char message[128];
            snprintf(message, sizeof(message), "{\"timestamp\": %u, \"sample\": %.2f}", timestamp, sample);

            // Invia al server WebSocket
            xQueueSend(dataQueue, &message, 0);
            lastSample = timestamp;
        }
    }
}
```

### **Script lato client per verifica**

Scrivi uno script Python per connetterti al WebSocket e verificare i dati ricevuti:

```python
import websocket
import json
import time

# Parametri
SAMPLE_RATE = 30000  # Frequenza di campionamento in Hz
EXPECTED_SAMPLES = 100  # Numero di campioni da ricevere
MAX_DEVIATION = 50  # Deviation massima in microsecondi

def on_message(ws, message):
    global received_samples, last_timestamp
    data = json.loads(message)
    timestamp = data["timestamp"]
    sample = data["sample"]

    if last_timestamp is not None:
        interval = timestamp - last_timestamp
        expected_interval = 1_000_000 // SAMPLE_RATE
        if abs(interval - expected_interval) > MAX_DEVIATION:
            print(f"Errore: intervallo errato {interval} us, atteso ~{expected_interval} us")
    last_timestamp = timestamp

    received_samples += 1
    if received_samples >= EXPECTED_SAMPLES:
        ws.close()
        print(f"Test completato: ricevuti {received_samples} campioni")

def on_error(ws, error):
    print(f"Errore: {error}")

def on_close(ws, close_status_code, close_msg):
    print("Connessione chiusa")

def on_open(ws):
    global received_samples, last_timestamp
    received_samples = 0
    last_timestamp = None
    print("Connessione aperta, inizio test")

# Connettiti al WebSocket
url = "ws://<ESP_IP>:81/data"
ws = websocket.WebSocketApp(url, on_message=on_message, on_error=on_error, on_close=on_close)
ws.on_open = on_open
ws.run_forever()
```

### Spiegazione del test

1. Generazione campioni: Il task ADC invia campioni con timestamp tramite WebSocket.
2. Script client: Lo script Python riceve i campioni, verifica il numero e controlla la frequenza.
3. Output: Lo script stampa errori nel caso di campioni mancanti o intervalli non corretti.

Risultati attesi
- Ogni campione ha un intervallo di circa 1_000_000 / SAMPLE_RATE microsecondi.
- Il numero totale di campioni ricevuti corrisponde a EXPECTED_SAMPLES.

Ecco un esempio di output dello script client Python durante l'esecuzione del test:

#### Output Normale (senza errori)

```plaintext
Connessione aperta, inizio test
Campione 1: sample=453.25, timestamp=0
Campione 2: sample=672.89, timestamp=33 us, intervallo=33 us (OK)
Campione 3: sample=198.54, timestamp=66 us, intervallo=33 us (OK)
Campione 4: sample=765.00, timestamp=99 us, intervallo=33 us (OK)
Campione 5: sample=390.12, timestamp=132 us, intervallo=33 us (OK)
...
Campione 100: sample=521.35, timestamp=3300 us, intervallo=33 us (OK)
Test completato: ricevuti 100 campioni
```

#### Output con errori (latenza o pacchetti mancanti)

```plaintext
Connessione aperta, inizio test
Campione 1: sample=453.25, timestamp=0
Campione 2: sample=672.89, timestamp=33 us, intervallo=33 us (OK)
Campione 3: sample=198.54, timestamp=80 us, intervallo=47 us (Errore: intervallo fuori tolleranza)
Campione 4: sample=765.00, timestamp=113 us, intervallo=33 us (OK)
Campione 5: sample=390.12, timestamp=160 us, intervallo=47 us (Errore: intervallo fuori tolleranza)
...
Errore: campione mancante! Atteso intervallo ~33 us, ricevuto 66 us
Campione 99: sample=672.45, timestamp=3234 us, intervallo=66 us (Errore: campione mancante!)
Campione 100: sample=521.35, timestamp=3267 us, intervallo=33 us (OK)
Test completato: ricevuti 100 campioni
```

Cosa significano i dati:
- **sample:** Il valore del campione ricevuto.
- **timestamp:** Il timestamp del campione in microsecondi.
- **intervallo:** La differenza di tempo tra due campioni consecutivi.
- **Errore:** Ogni volta che l'intervallo supera la tolleranza impostata (MAX_DEVIATION), lo script segnala un errore o un campione mancante.

## **Versione per Arduino IDE**

```C++
#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>

// Configurazione hardware
#define CS_PIN 5
#define DRDY_PIN 4

// Configurazione acquisizione
#define DEFAULT_SAMPLE_RATE 30000 // Hz

// Configurazione Wi-Fi
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";

// Gestione configurazione
struct Config {
    uint32_t sampleRate;
    uint8_t gain;
    bool filterEnabled;
    float threshold;
    bool streaming;
};

// Variabili globali
QueueHandle_t dataQueue;
QueueHandle_t eventQueue;
AsyncWebServer server(80);
AsyncWebSocket wsData("/data");
AsyncWebSocket wsEvents("/events");
Config globalConfig = {DEFAULT_SAMPLE_RATE, 1, false, 1000000, true};
float emaAlpha = 0.1; // Coefficiente EMA

// Gestione eventi WebSocket
void onEventsWebSocket(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("Client eventi %u connesso\n", client->id());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("Client eventi %u disconnesso\n", client->id());
    } else if (type == WS_EVT_DATA) {
        String message = String((char*)data).substring(0, len);
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, message);
        
        if (!error) {
            if (doc.containsKey("samplerate")) {
                globalConfig.sampleRate = doc["samplerate"].as<int>();
                Serial.printf("Sample rate impostato a: %d\n", globalConfig.sampleRate);
            }
            
            if (doc.containsKey("alfaema")) {
                emaAlpha = doc["alfaema"].as<float>();
                Serial.printf("EMA alpha impostato a: %.2f\n", emaAlpha);
            }
        }
    }
}

// Task acquisizione dati ADC (Core 0)
void IRAM_ATTR adcTask(void* pvParameters) {
    SPIClass spi(VSPI);
    spi.begin(18, 19, 23, CS_PIN);
    pinMode(CS_PIN, OUTPUT);
    pinMode(DRDY_PIN, INPUT);

    uint32_t lastSample = 0;
    uint32_t sampleInterval = 1000000 / globalConfig.sampleRate;
    float emaFilteredValue = 0.0;

    while (true) {
        if (!globalConfig.streaming) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        if (digitalRead(DRDY_PIN) == LOW &&
            (micros() - lastSample) >= sampleInterval) {

            digitalWrite(CS_PIN, LOW);
            uint8_t data[3] = {0x01, 0x02, 0x03};
            digitalWrite(CS_PIN, HIGH);

            int32_t value = (data[0] << 16) | (data[1] << 8) | data[2];
            if (value & 0x800000) value -= 0x1000000;

            emaFilteredValue = emaAlpha * value + (1 - emaAlpha) * emaFilteredValue;

            xQueueSend(dataQueue, &emaFilteredValue, 0);
            lastSample = micros();
        }
        taskYIELD();  // Permette altri task critici sul core 0 se necessario
    }
}

// Task gestione dati WebSocket (Core 1)
void webSocketDataTask(void* pvParameters) {
    float value;
    char buffer[32];
    
    while(true) {
        if(xQueueReceive(dataQueue, &value, portMAX_DELAY) == pdTRUE) {
            if(wsData.count() > 0) {  // Se ci sono client connessi
                snprintf(buffer, sizeof(buffer), "%.2f", value);
                wsData.textAll(buffer);
            }
        }
    }
}

void setup() {
    Serial.begin(115200);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnesso al WiFi");

    // Crea le code
    dataQueue = xQueueCreate(1024, sizeof(float));
    eventQueue = xQueueCreate(32, sizeof(char*));

    // Configurazione WebSocket
    wsEvents.onEvent(onEventsWebSocket);
    server.addHandler(&wsEvents);
    server.addHandler(&wsData);
    server.begin();

    // Avvia i task sui core specifici
    xTaskCreatePinnedToCore(adcTask, "ADC Task", 8192, NULL, 1, NULL, 0);        // Core 0
    xTaskCreatePinnedToCore(webSocketDataTask, "WS Data Task", 8192, NULL, 1, NULL, 1);  // Core 1

    Serial.println("Sistema inizializzato");
}

void loop() {
    vTaskDelay(1);  // Previene watchdog reset
}
```

>[Torna all'indice](readme.md#fasi-progetto)
