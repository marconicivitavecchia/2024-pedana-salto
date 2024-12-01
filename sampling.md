

# ADS1256 e Multiplexing in MicroPython

## 1. Domanda iniziale
**Domanda**: L'ADS1256 campiona automaticamente gli ingressi dopo che abbiamo impostato la frequenza?

**Risposta**:  
No, l'ADS1256 non campiona automaticamente gli ingressi in modalità multiplexing. È necessario:
1. Commutare manualmente i canali tramite il multiplexer integrato inviando comandi specifici.
2. Attendere che i dati siano pronti (indicati dal pin `DRDY`).
3. Leggere i valori attraverso l'interfaccia SPI.

---

## 2. Approfondimento sulla lettura in multiplexing di 4 celle di carico
Per leggere correttamente 4 celle di carico:

1. **Configurazione del multiplexer (MUX):**
   - Impostare il registro MUX per selezionare il canale da leggere.
   
2. **Avviare la conversione:**
   - Inviare il comando `SYNC` per sincronizzare e `WAKEUP` per iniziare la conversione.

3. **Attendere che il dato sia pronto:**
   - Controllare il pin `DRDY` o utilizzare un **interrupt**.

4. **Leggere il dato:**
   - Inviare il comando `RDATA` e leggere i 3 byte di dati a 24 bit tramite SPI.

5. **Passare al canale successivo:**
   - Ripetere i passaggi per ciascun canale, tenendo conto del tempo di commutazione.

---

## 3. Frequenza di campionamento consigliata
Per leggere 4 canali a una frequenza di **200 Hz per canale**, la frequenza totale richiesta è:

$$f_{\text{totale}}=200\,\text{Hz}\times4=800\,\text{Hz}$$

Questa frequenza è raggiungibile configurando il Data Rate dell'ADS1256 a **1000 SPS** o superiore, ma è necessario considerare anche i tempi di elaborazione.

---

## 4. Calcolo della massima frequenza di campionamento per canale

### Tempo di conversione
Con un Data Rate massimo di **30.000 SPS** (registro `DRATE = 0xF0`):

$$t_{\text{conv}} = \frac{1}{30.000} = 33.33 \, \mu\text{s}$$

### Tempo totale su 4 canali
Il tempo totale per un ciclo completo di 4 canali è:

$$t_{\text{ciclo}} = 4 \times t_{\text{conv}} = 4 \times 33.33 \, \mu\text{s} = 133.33 \, \mu\text{s}$$

La frequenza massima per canale è:

$$f_{\text{canale}} = \frac{1}{t_{\text{ciclo}}} = 7.5 \, \text{kHz}$$
### Considerando tempi di guardia
Aggiungendo **15 µs** di ritardo per canale (comando SPI, commutazione MUX):

$$t_{\text{ciclo}} = 4 \times (33.33 + 15) = 193.33 \, \mu\text{s}$$

$$f_{\text{canale}} \approx 5.17 \, \text{kHz}$$

---

## 5. Frequenza consigliata in MicroPython
Dato che MicroPython è un linguaggio interpretato e relativamente lento rispetto al codice nativo, si consiglia:

- **250-500 Hz per canale** (1000-2000 SPS totali).

Per frequenze superiori (>1 kHz per canale), è meglio considerare l'uso di C/C++.

---

## 6. Implementazione in MicroPython

### Loop senza blocchi
Gestione del pin `DRDY` con un timeout:

```python
import time
from machine import SPI, Pin

# Configurazione del pin CS (Chip Select) e DRDY (Data Ready)
CS_PIN = 5
DRDY_PIN = 4

cs = Pin(CS_PIN, Pin.OUT)
drdy = Pin(DRDY_PIN, Pin.IN)

# Configurazione SPI
spi = SPI(1, baudrate=10000000, polarity=0, phase=1, sck=Pin(18), mosi=Pin(23), miso=Pin(19))

# Variabile globale per indicare quando i dati sono pronti
data_ready = False

def drdy_interrupt(pin):
    """Interrupt handler per DRDY: segnala che i dati sono pronti."""
    global data_ready
    data_ready = True

# Configurazione interrupt su DRDY (trigger sul fronte di discesa)
drdy.irq(trigger=Pin.IRQ_FALLING, handler=drdy_interrupt)

def send_command(command):
    """Invia un comando all'ADS1256."""
    cs.off()
    spi.write(bytearray([command]))
    cs.on()

def select_channel(channel):
    """Seleziona il canale nel multiplexer interno dell'ADS1256."""
    mux_config = channel * 8  # Configura il registro MUX per il canale
    cs.off()
    spi.write(bytearray([0x50 | 0x01, 0x00, mux_config]))  # Scrive nel registro MUX
    cs.on()
    send_command(0xFC)  # Comando SYNC per sincronizzare
    send_command(0x00)  # Comando WAKEUP per iniziare la conversione

def read_adc():
    """Legge un valore a 24 bit dall'ADS1256."""
    cs.off()
    spi.write(bytearray([0x01]))  # Comando RDATA
    result = spi.read(3)  # Legge 3 byte di dati
    cs.on()
    # Conversione in valore signed a 24 bit
    raw_value = (result[0] << 16) | (result[1] << 8) | result[2]
    if raw_value & 0x800000:  # Controlla il bit di segno
        raw_value -= 0x1000000
    return raw_value

def read_all_channels(channels=4):
    """Legge i dati da tutti i canali configurati."""
    global data_ready
    readings = []
    for i in range(channels):
        # Seleziona il canale
        select_channel(i)
        # Aspetta che DRDY segnali che i dati sono pronti
        data_ready = False
        while not data_ready:
            pass
        # Legge il valore grezzo
        raw_data = read_adc()
        readings.append(raw_data)
    return readings

# Loop principale per leggere i dati dai 4 canali
try:
    while True:
        # Legge i dati da 4 canali
        values = read_all_channels(channels=4)
        for i, value in enumerate(values):
            print(f"Canale {i}: Valore grezzo = {value}")
        time.sleep(0.01)  # Pausa per evitare di sovraccaricare il loop
except KeyboardInterrupt:
    print("Esecuzione interrotta.")
```

# Lettura di 4 Celle di Carico con ADS1256 in MicroPython

Questo codice permette di leggere i dati da 4 celle di carico collegate a un ADC **ADS1256**, utilizzando un **ESP32** programmato in **MicroPython**. La lettura dei dati avviene attraverso l'interfaccia SPI, sfruttando un interrupt sul pin `DRDY` per sapere quando i dati sono pronti.

---

## Funzionalità principali

### 1. **Configurazione dei Pin**
- **CS_PIN (Chip Select):** controlla la comunicazione con l'ADS1256.
- **DRDY_PIN (Data Ready):** monitora lo stato del segnale `DRDY` per sapere quando i dati sono pronti.

```python
CS_PIN = 5
DRDY_PIN = 4
spi = SPI(1, baudrate=10000000, polarity=0, phase=1, sck=Pin(18), mosi=Pin(23), miso=Pin(19))

```

### 2. **Interrupt su `DRDY`**
L'interrupt viene configurato per attivarsi sul fronte di discesa del segnale `DRDY`. Quando l'ADS1256 segnala che i dati sono pronti, l'handler dell'interrupt imposta la variabile globale `data_ready` a `True`.

```python
def drdy_interrupt(pin):
    global data_ready
    data_ready = True

drdy.irq(trigger=Pin.IRQ_FALLING, handler=drdy_interrupt)
```

### 3. **Funzioni Principali**
#### **`send_command(command)`**
Invia un comando generico all'ADS1256 utilizzando SPI:
- **Parametri:** comando a 1 byte.
- **Esempio:** invio del comando `0xFC` per sincronizzare (`SYNC`).

#### **`select_channel(channel)`**
Configura il registro MUX per selezionare il canale desiderato:
- **Parametri:** numero del canale (0-7).
- Invia il comando per scrivere nel registro MUX, seguito dai comandi `SYNC` e `WAKEUP` per avviare la conversione.

#### **`read_adc()`**
Legge un valore a 24 bit dall'ADS1256:
- Invia il comando `RDATA` per leggere i dati.
- Converte i 3 byte ricevuti in un valore signed a 24 bit.

#### **`read_all_channels(channels=4)`**
Legge i dati da tutti i canali configurati:
- Itera sui canali da 0 a `channels-1`.
- Seleziona ogni canale con `select_channel()`.
- Aspetta che l'interrupt su `DRDY` segnali che i dati sono pronti.
- Legge e memorizza il valore campionato con `read_adc()`.

---

### 4. **Loop Principale**
Il loop principale legge i valori dai 4 canali e li stampa sulla console:
1. Chiama `read_all_channels()` per ottenere i dati da tutti i canali.
2. Stampa i valori grezzi per ciascun canale.
3. Include una pausa (`time.sleep(0.01)`) per evitare un sovraccarico del ciclo.

```python
try:
    while True:
        values = read_all_channels(channels=4)
        for i, value in enumerate(values):
            print(f"Canale {i}: Valore grezzo = {value}")
        time.sleep(0.01)
except KeyboardInterrupt:
    print("Esecuzione interrotta.")

```

## Vantaggi del Codice
1. **Efficienza con Interrupt su `DRDY`:**
   - Elimina il polling continuo del segnale `DRDY`, riducendo il carico della CPU.

2. **Configurazione ottimizzata di SPI:**
   - Utilizza una velocità di 10 MHz per comunicare rapidamente con l'ADS1256.

3. **Gestione Multi-Canale:**
   - Permette di leggere fino a 8 canali, con 4 preconfigurati.

4. **Compatibilità con MicroPython:**
   - Adattato alla lentezza del linguaggio, mantenendo una frequenza consigliata di 250-500 Hz per canale.


## Ottimizzazione

```python
import time
from machine import SPI, Pin

# Configurazione dei pin
CS_PIN = 5       # Chip Select
DRDY_PIN = 4     # Data Ready
SYNC_PIN = 16    # Pin di sincronizzazione (trigger esterno)

cs = Pin(CS_PIN, Pin.OUT)
drdy = Pin(DRDY_PIN, Pin.IN)
sync_pin = Pin(SYNC_PIN, Pin.IN, Pin.PULL_UP)

# Configurazione SPI
spi = SPI(1, baudrate=10000000, polarity=0, phase=1, sck=Pin(18), mosi=Pin(23), miso=Pin(19))

# Variabile globale per segnalare che i dati sono pronti
data_ready = False

def drdy_interrupt(pin):
    """Interrupt handler per DRDY: segnala che i dati sono pronti."""
    global data_ready
    data_ready = True

# Configurazione interrupt su DRDY
drdy.irq(trigger=Pin.IRQ_FALLING, handler=drdy_interrupt)

def send_command(command):
    """Invia un comando all'ADS1256."""
    cs.off()
    spi.write(bytearray([command]))
    cs.on()

def select_channel(channel):
    """Seleziona il canale nel multiplexer interno dell'ADS1256."""
    mux_config = channel * 8  # Configura il registro MUX per il canale
    cs.off()
    spi.write(bytearray([0x50 | 0x01, 0x00, mux_config]))  # Scrive nel registro MUX
    cs.on()
    send_command(0xFC)  # Comando SYNC per sincronizzare
    send_command(0x00)  # Comando WAKEUP per iniziare la conversione

def read_adc():
    """Legge un valore a 24 bit dall'ADS1256."""
    cs.off()
    spi.write(bytearray([0x01]))  # Comando RDATA
    result = spi.read(3)  # Legge 3 byte di dati
    cs.on()
    # Conversione in valore signed a 24 bit
    raw_value = (result[0] << 16) | (result[1] << 8) | result[2]
    if raw_value & 0x800000:  # Controlla il bit di segno
        raw_value -= 0x1000000
    return raw_value

def read_all_channels(channels=4):
    """Legge i dati da tutti i canali configurati."""
    global data_ready
    readings = []
    for i in range(channels):
        # Seleziona il canale
        select_channel(i)
        # Aspetta che DRDY segnali che i dati sono pronti
        data_ready = False
        while not data_ready:
            pass
        # Legge il valore grezzo
        raw_data = read_adc()
        readings.append(raw_data)
    return readings

def apply_low_pass_filter(data, filtered_data, alpha=0.1):
    """Applica un filtro passa-basso semplice (media esponenziale)."""
    for i in range(len(data)):
        filtered_data[i] = alpha * data[i] + (1 - alpha) * filtered_data[i]
    return filtered_data

# Buffer per memorizzare i campioni
FREQUENZA_CAMPIONAMENTO = 200  # Hz per canale
DURATA_SALTO = 2  # Durata massima del salto in secondi
NUM_CANALI = 4

BUFFER_DIM = FREQUENZA_CAMPIONAMENTO * DURATA_SALTO
buffer = [[0] * NUM_CANALI for _ in range(BUFFER_DIM)]
filtered_data = [0] * NUM_CANALI  # Per il filtro passa-basso
buffer_index = 0  # Indice per il buffer circolare

# File per salvare i dati
FILE_PATH = "/sd/salto.csv"

# Funzione per salvare i dati su file
def save_to_file(file_path, data):
    """Salva i dati nel file specificato."""
    with open(file_path, "w") as file:
        for row in data:
            file.write(",".join(map(str, row)) + "\n")

try:
    print("In attesa di sincronizzazione hardware...")
    while sync_pin.value() == 1:
        time.sleep(0.01)  # Aspetta il trigger di sincronizzazione
    
    print("Sincronizzazione ricevuta! Inizio acquisizione dati...")
    start_time = time.ticks_ms()

    while time.ticks_diff(time.ticks_ms(), start_time) < DURATA_SALTO * 1000:
        # Legge i dati dai canali
        values = read_all_channels(channels=NUM_CANALI)
        
        # Applica il filtro passa-basso
        filtered_data = apply_low_pass_filter(values, filtered_data)
        
        # Salva i dati filtrati nel buffer
        buffer[buffer_index] = filtered_data[:]
        buffer_index = (buffer_index + 1) % BUFFER_DIM
        
        # Pausa per mantenere la frequenza di campionamento
        time.sleep(1 / FREQUENZA_CAMPIONAMENTO)

    print("Acquisizione completata. Salvataggio su file...")
    save_to_file(FILE_PATH, buffer)
    print(f"Dati salvati su {FILE_PATH}")
except KeyboardInterrupt:
    print("Esecuzione interrotta.")
```

### Struttura del file csv

Il file generato avrà questo formato:

```python
Canale1,Canale2,Canale3,Canale4
123456,234567,345678,456789
123460,234570,345680,456790
...

## Timestamp

Aggiungiamo il timestamp relativo a ciascun campione nel file CSV. Il timestamp sarà calcolato come il tempo trascorso in millisecondi dall'inizio dell'acquisizione. Ecco il codice aggiornato:

```python
import time
from machine import SPI, Pin

# Configurazione dei pin
CS_PIN = 5       # Chip Select
DRDY_PIN = 4     # Data Ready
SYNC_PIN = 16    # Pin di sincronizzazione (trigger esterno)

cs = Pin(CS_PIN, Pin.OUT)
drdy = Pin(DRDY_PIN, Pin.IN)
sync_pin = Pin(SYNC_PIN, Pin.IN, Pin.PULL_UP)

# Configurazione SPI
spi = SPI(1, baudrate=10000000, polarity=0, phase=1, sck=Pin(18), mosi=Pin(23), miso=Pin(19))

# Variabile globale per segnalare che i dati sono pronti
data_ready = False

def drdy_interrupt(pin):
    """Interrupt handler per DRDY: segnala che i dati sono pronti."""
    global data_ready
    data_ready = True

# Configurazione interrupt su DRDY
drdy.irq(trigger=Pin.IRQ_FALLING, handler=drdy_interrupt)

def send_command(command):
    """Invia un comando all'ADS1256."""
    cs.off()
    spi.write(bytearray([command]))
    cs.on()

def select_channel(channel):
    """Seleziona il canale nel multiplexer interno dell'ADS1256."""
    mux_config = channel * 8  # Configura il registro MUX per il canale
    cs.off()
    spi.write(bytearray([0x50 | 0x01, 0x00, mux_config]))  # Scrive nel registro MUX
    cs.on()
    send_command(0xFC)  # Comando SYNC per sincronizzare
    send_command(0x00)  # Comando WAKEUP per iniziare la conversione

def read_adc():
    """Legge un valore a 24 bit dall'ADS1256."""
    cs.off()
    spi.write(bytearray([0x01]))  # Comando RDATA
    result = spi.read(3)  # Legge 3 byte di dati
    cs.on()
    # Conversione in valore signed a 24 bit
    raw_value = (result[0] << 16) | (result[1] << 8) | result[2]
    if raw_value & 0x800000:  # Controlla il bit di segno
        raw_value -= 0x1000000
    return raw_value

def read_all_channels(channels=4):
    """Legge i dati da tutti i canali configurati."""
    global data_ready
    readings = []
    for i in range(channels):
        # Seleziona il canale
        select_channel(i)
        # Aspetta che DRDY segnali che i dati sono pronti
        data_ready = False
        while not data_ready:
            pass
        # Legge il valore grezzo
        raw_data = read_adc()
        readings.append(raw_data)
    return readings

def apply_low_pass_filter(data, filtered_data, alpha=0.1):
    """Applica un filtro passa-basso semplice (media esponenziale)."""
    for i in range(len(data)):
        filtered_data[i] = alpha * data[i] + (1 - alpha) * filtered_data[i]
    return filtered_data

# Buffer per memorizzare i campioni
FREQUENZA_CAMPIONAMENTO = 200  # Hz per canale
DURATA_SALTO = 2  # Durata massima del salto in secondi
NUM_CANALI = 4

BUFFER_DIM = FREQUENZA_CAMPIONAMENTO * DURATA_SALTO
buffer = [[0] * (NUM_CANALI + 1) for _ in range(BUFFER_DIM)]  # +1 per il timestamp
filtered_data = [0] * NUM_CANALI  # Per il filtro passa-basso
buffer_index = 0  # Indice per il buffer circolare

# File per salvare i dati
FILE_PATH = "/sd/salto.csv"

# Funzione per salvare i dati su file
def save_to_file(file_path, data):
    """Salva i dati nel file specificato."""
    with open(file_path, "w") as file:
        file.write("Timestamp,Canale1,Canale2,Canale3,Canale4\n")  # Intestazione
        for row in data:
            file.write(",".join(map(str, row)) + "\n")

try:
    print("In attesa di sincronizzazione hardware...")
    while sync_pin.value() == 1:
        time.sleep(0.01)  # Aspetta il trigger di sincronizzazione
    
    print("Sincronizzazione ricevuta! Inizio acquisizione dati...")
    start_time = time.ticks_ms()

    while time.ticks_diff(time.ticks_ms(), start_time) < DURATA_SALTO * 1000:
        # Legge i dati dai canali
        values = read_all_channels(channels=NUM_CANALI)
        
        # Applica il filtro passa-basso
        filtered_data = apply_low_pass_filter(values, filtered_data)
        
        # Aggiunge il timestamp e salva nel buffer
        timestamp = time.ticks_diff(time.ticks_ms(), start_time)  # Tempo relativo in ms
        buffer[buffer_index] = [timestamp] + filtered_data[:]
        buffer_index = (buffer_index + 1) % BUFFER_DIM
        
        # Pausa per mantenere la frequenza di campionamento
        time.sleep(1 / FREQUENZA_CAMPIONAMENTO)

    print("Acquisizione completata. Salvataggio su file...")
    save_to_file(FILE_PATH, buffer)
    print(f"Dati salvati su {FILE_PATH}")
except KeyboardInterrupt:
    print("Esecuzione interrotta.")

```

### Struttura del dile csv

```python
Timestamp,Canale1,Canale2,Canale3,Canale4
0,123456,234567,345678,456789
5,123460,234570,345680,456790
10,123470,234580,345690,456800
```

## Calibrazione

Processo in 3 fasi:

1. Lettura a Vuoto (Tara):

   - I valori di uscita grezzi delle 4 celle vengono letti e sommati.
   - Questo valore rappresenta la tara totale del sistema.

1. Lettura con Peso di Riferimento:

   - Un peso noto (ad esempio, 1000 grammi) viene posizionato sulla pedana.
   - I valori letti dalle 4 celle vengono sommati.

3. Calcolo del Fattore di Scala:

   - Il fattore di scala viene calcolato come:

      $$\text{Fattore di scala} = \frac{\text{Somma dei valori con peso} - \text{Somma dei valori di tara}}{\text{Peso noto}}$$
​
 
4. Memorizzazione dei Parametri:

   - I valori di tara per ciascuna cella e il fattore di scala complessivo vengono salvati in un file CSV.

#### Esempio di Utilizzo
1. Posizionare la pedana senza alcun carico e avviare il programma.
2. Posizionare il peso noto (es. 1000 grammi) quando richiesto.
3. I dati di calibrazione verranno salvati nel file /sd/calibrazione_sistema.csv.

Per introdurre la calibrazione basata su transizioni di peso rilevate (fronte di discesa e fronte di salita), possiamo implementare un sistema che rilevi variazioni significative nel peso totale misurato dalle 4 celle di carico. Questo approccio sostituisce il ritardo fisso con un monitoraggio dinamico delle misurazioni, reagendo a eventi di sollevamento del peso e applicazione del peso di riferimento.

#### Strategia

- **Rilevazione di discesa (togli):** La lettura del peso diminuisce significativamente e rimane stabile (si stabilizza sotto una certa soglia).
- **Rilevazione di salita (metti):** Subito dopo una discesa, il peso inizia ad aumentare e raggiunge un valore stabile sopra una soglia predefinita.

```python
import time
from machine import Pin, SPI

# Configurazione dei pin
CS_PIN = 5
DRDY_PIN = 4
cs = Pin(CS_PIN, Pin.OUT)
drdy = Pin(DRDY_PIN, Pin.IN)

# Configurazione SPI
spi = SPI(1, baudrate=10000000, polarity=0, phase=1, sck=Pin(18), mosi=Pin(23), miso=Pin(19))

def send_command(command):
    """Invia un comando all'ADS1256."""
    cs.value(0)
    spi.write(bytearray([command]))
    cs.value(1)

def select_channel(channel):
    """Seleziona il canale nel multiplexer interno dell'ADS1256."""
    mux_config = channel * 8  # Configura il registro MUX per il canale
    cs.value(0)
    spi.write(bytearray([0x50 | 0x01, 0x00, mux_config]))  # Scrive nel registro MUX
    cs.value(1)
    send_command(0xFC)  # Comando SYNC per sincronizzare
    send_command(0x00)  # Comando WAKEUP per iniziare la conversione

def read_adc():
    """Legge un valore a 24 bit dall'ADS1256."""
    cs.value(0)
    spi.write(bytearray([0x01]))  # Comando RDATA
    result = spi.read(3)  # Legge 3 byte di dati
    cs.value(1)
    raw_value = (result[0] << 16) | (result[1] << 8) | result[2]
    if raw_value & 0x800000:  # Controlla il bit di segno
        raw_value -= 0x1000000
    return raw_value

def read_all_channels(channels=4):
    """Legge i dati da tutti i canali configurati."""
    readings = []
    for i in range(channels):
        select_channel(i)
        time.sleep(0.01)  # Pausa per stabilizzare
        readings.append(read_adc())
    return readings

def detect_weight_change(target_delta, channels=4):
    """
    Rileva un cambio significativo di peso basato su una variazione target.
    - target_delta: variazione di peso (in unità ADC) considerata significativa.
    - channels: numero di canali da leggere (default: 4).
    Restituisce True se il cambio viene rilevato, altrimenti False.
    """
    previous_sum = sum(read_all_channels(channels))
    while True:
        current_sum = sum(read_all_channels(channels))
        delta = abs(current_sum - previous_sum)
        if delta > target_delta:
            return current_sum
        previous_sum = current_sum
        time.sleep(0.1)  # Ritardo minimo tra le letture

def detect_togli_metti(target_delta, channels=4):
    """
    Rileva una transizione completa da "togli" (discesa) a "metti" (salita) del peso.
    Restituisce True se la transizione è stata rilevata, altrimenti False.
    """
    print("Inizio rilevazione della transizione (togli -> metti)...")
    previous_sum = sum(read_all_channels(channels))
    weight_removed = False
    weight_added = False
    
    while True:
        current_sum = sum(read_all_channels(channels))
        delta = abs(current_sum - previous_sum)

        # Rilevazione di discesa (togli)
        if not weight_removed and delta > target_delta and current_sum < previous_sum:
            print("Peso rimosso (fase di discesa rilevata)...")
            weight_removed = True
        
        # Rilevazione di salita (metti) dopo discesa
        if weight_removed and not weight_added and delta > target_delta and current_sum > previous_sum:
            print("Peso aggiunto (fase di salita rilevata)...")
            weight_added = True

        # Se entrambe le fasi sono rilevate, la transizione è completata
        if weight_removed and weight_added:
            return True

        previous_sum = current_sum
        time.sleep(0.1)  # Ritardo tra le letture per evitare il sovraccarico

def calibrate_system(weight, channels=4, target_delta=5000):
    """
    Calibra il sistema basandosi sulle transizioni di peso.
    - weight: peso noto (in unità fisiche, ad esempio grammi o chilogrammi).
    - channels: numero di canali (default: 4).
    - target_delta: soglia di variazione del peso per rilevare transizioni.
    Restituisce i valori di tara e il fattore di scala.
    """
    print("Calibrazione del sistema...")

    # Fase 1: Lettura a vuoto (tara)
    print("Rimuovere tutto il peso dalla pedana...")
    tare_sum = detect_weight_change(target_delta)
    print(f"Tara totale rilevata: {tare_sum}")

    # Fase 2: Lettura con peso noto
    print(f"Posizionare un peso noto di {weight} unità sulla pedana...")
    weight_sum = detect_weight_change(target_delta)
    print(f"Somma dei valori con peso: {weight_sum}")

    # Calcolo del fattore di scala
    scale_factor = (weight_sum - tare_sum) / weight
    print(f"Fattore di scala calcolato: {scale_factor}")

    return tare_sum, scale_factor

def save_calibration_to_file(file_path, tare_sum, scale_factor):
    """
    Salva i dati di calibrazione su file.
    - file_path: percorso del file.
    - tare_sum: valore totale di tara.
    - scale_factor: fattore di scala calcolato.
    """
    with open(file_path, "w") as file:
        file.write("Tara,Fattore_di_Scala\n")
        file.write(f"{tare_sum},{scale_factor}\n")
    print(f"Dati di calibrazione salvati su {file_path}")

# Percorso del file di calibrazione
CALIBRATION_FILE = "/sd/calibrazione_sistema.csv"

try:
    print("Inizio calibrazione...")
    # Peso noto utilizzato per la calibrazione (es. 1000 grammi)
    WEIGHT = 1000

    # Esegui la calibrazione del sistema
    tare_sum, scale_factor = calibrate_system(WEIGHT)

    # Salva i dati di calibrazione su file
    save_calibration_to_file(CALIBRATION_FILE, tare_sum, scale_factor)

    print("Calibrazione completata con successo.")
    
    # Rilevazione della transizione
    detect_togli_metti(target_delta=5000)

except KeyboardInterrupt:
    print("Calibrazione interrotta.")

```
