

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
