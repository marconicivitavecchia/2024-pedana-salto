## Specifiche Cella di Carico YZC-516C

## Capacità e Range
- Range disponibili: 0.1, 0.2, 0.3, 0.5, 1, 2 tonnellate

## Specifiche Elettriche
- Tensione di uscita: 2.0±0.05 mV/V
- Resistenza di ingresso: 365±5 Ω
- Resistenza di uscita: 350±3 Ω
- Resistenza di isolamento: >5000 MΩ/50V
- Tensione di eccitazione raccomandata: 5~12V

## Prestazioni e Precisione
- Classe di precisione: C2
- Errore combinato: ≤ ±0.030
- Creep: 0.02
- Effetto temperatura su sensibilità: 0.0017 %RO/°C
- Effetto temperatura su zero: 0.005 %RO/°C
- Bilanciamento zero: ±1.0 %RO

## Range di Temperatura
- Range temperatura compensata: -10~+40°C
- Range temperatura operativa: -30~+80°C

## Sovraccarichi
- Sovraccarico di sicurezza: 150% RO
- Sovraccarico ultimo: 200% RO

## Specifiche Meccaniche
- Materiale: Lega di acciaio
- Cavo: Schermato Ø5.0X3.5m

## Dimensioni
| Capacità    | B (mm) | D          |
|-------------|--------|------------|
| 100-300kg   | 19.05  | M12x1.75   |
| 500kg       | 25.4   | M12x1.75   |
| 1t-2t       | 25.4   | M16x2      |

## Caratteristiche Speciali
- Alta resistenza all'acqua
- Cavo schermato standard
- Connessione aerea su richiesta

## ADS1256 - Specifiche Tecniche

### Caratteristiche Principali
- Risoluzione: 24 bit
- Velocità di campionamento massima: 30 ksps
- Canali di ingresso: 8
- Interfaccia: SPI
- Architettura: Delta-Sigma
- Rating: Catalog

### Configurazione Ingressi
- Tipo: Differenziale o Single-ended
- Configurazione multicanale: Multiplexata
- Range tensione di ingresso: 0V - 5.25V
- Modalità di riferimento: Esterna

### Alimentazione
- Consumo tipico: 36 mW
- Alimentazione analogica: 4.75V - 5.25V
- Alimentazione digitale: 1.8V - 3.6V

### Caratteristiche Operative
- Range temperatura operativa: -40°C a +85°C
- Caratteristiche speciali:
  - Reiezione 50/60 Hz
  - GPIO
  - PGA (Amplificatore a Guadagno Programmabile)

# CALCOLO DEL GUADAGNO DELL’ADC

## Formula ADS1256 Output Code

Il codice di uscita dell'ADS1256 è dato da:

$$Code = 2^{23} \cdot \frac{(V_+ - V_-)}{2V_{REF}}$$

Dove:
- $V_+$ è la tensione sull'ingresso positivo
- $V_-$ è la tensione sull'ingresso negativo
- $V_{REF}$ è la tensione di riferimento
- $2^{23}$ rappresenta metà della risoluzione totale (24 bit)

## Formula ADS1256 consigliata per ingressi single ended (non duali)

Il codice di uscita dell'ADS1256 è dato da:

$$Code = 2^{23} \cdot \frac{(V_+ - V_{REF})}{2V_{REF}} = 2^{23}\frac{2} \cdot (\frac{(V_+}{V_{REF}} - 1)}$$

Dove:
- $V_+$ è la tensione sull'ingresso positivo
- $V_-$ è la tensione sull'ingresso negativo
- $V_{REF}$ è la tensione di riferimento
- $2^{23}$ rappresenta metà della risoluzione totale (24 bit)

Per una cella tipica:
- Alimentazione: 5V
- Sensibilità: 2mV/V dove V sono i volt di alimentazione
- Carico nominale: 100kg

Se supponiamo una alimentazione di 5V:

Signal_max = 5V * (2mV/V) = 10mV a fondo scala

Quindi se ADC ha range ±2.5V: G = 2.5V/10mV = 250

ADS1256
Il guadagno più vicino disponibile nell'ADS1256 andrebbe scelto per utilizzare la maggior parte del range mantenendo un margine di sicurezza.
L'ADS1256 ha un range di ingresso di riferimento di ±VREF, dove VREF è tipicamente 2.5V.
Quindi con sensibilità 2mV/V e Vcc=5V:
•	Output cella a fondo scala = 5V * 2mV/V = 10mV
•	Range ADC = ±2.5V
•	Guadagno teorico = 2.5V/10mV = 250
Ma ADS1256 offre solo guadagni: 1,2,4,8,16,32,64
In questo caso si sceglierebbe G=64 come massimo disponibile, e si compenserà via software la differenza rimanente nella calibrazione.
La risoluzione effettiva sarà: (10mV * 64)/(2.5V) = 25.6% del range ADC
Analizziamo i dati dalla scheda tecnica delle celle di carico:
•	Sensibilità: 2.0±0.05 mV/V
•	Tensione eccitazione raccomandata: 5-12V
Usando 5V di eccitazione: Output massimo = 5V * 2mV/V = 10mV
Con ADS1256 (range ±2.5V): Guadagno ideale = 2.5V/10mV = 250
Poiché ADS1256 ha guadagni disponibili 1,2,4,8,16,32,64, dovremmo usare G=64 come massimo disponibile.

Con questo guadagno:
•	Segnale amplificato max = 10mV * 64 = 640mV
•	Utilizzo range ADC = 640mV/2500mV ≈ 25.6%
La rimanente amplificazione necessaria (circa 4x) dovrà essere gestita via software nella calibrazione.
L'ADS1256 è un ADC a 24 bit, quindi:
Range totale = 2^24 = 16,777,216 livelli
25.6% del range = 16,777,216 * 0.256 = 4,295,167 livelli
Considerando che per misure di peso servono tipicamente 50,000-100,000 livelli per una buona risoluzione, anche utilizzando solo il 25.6% del range abbiamo ancora una risoluzione più che adeguata (>4 milioni di livelli).
Per una cella da 300kg questo significa: 4,295,167/300 ≈ 14,317 livelli/kg ≈ 0.07g di risoluzione teorica
Dalla scheda tecnica:
•	n max (numero massimo di divisioni) = 2000
•	Accuracy class: C2
Quindi anche se l'ADC ha una risoluzione molto più alta (4 milioni di livelli nel 25.6% del range), il limite reale è dato dalla cella stessa che può garantire solo 2000 divisioni certificate.
Per una cella da 300kg significa: 300kg/2000 = 0.15kg = 150g di risoluzione effettiva reale
Tutto il resto della risoluzione dell'ADC (14,317 livelli/kg) è "sprecato" perché la cella non può garantire quella precisione. È un classico caso dove la risoluzione del sistema di misura (ADC) è molto superiore alla precisione del sensore (cella).

NON LINEARITA’ CELLA
Dalla scheda tecnica:
Combined error < ±0.030%RO
Questo include sia non linearità che isteresi.
Dividendo ulteriormente:
- Creep (deriva) = 0.02%RO 
- Zero balance = ±1.0%RO
Per una cella da 300kg significa:
- Errore combinato max: ±0.03% * 300kg = ±0.09kg
- Creep: 0.02% * 300kg = 0.06kg
- Errore di zero: ±1% * 300kg = ±3kg

Ci sono anche gli effetti della temperatura:
- Sulla sensibilità: 0.0017%RO/°C
- Sullo zero: 0.005%RO/°C
Quindi è una cella di classe C2 con prestazioni tipiche per applicazioni industriali/commerciali, dove l'isteresi e non linearità sono mantenute entro lo 0.03% del fondo scala.

NON LINEARITA’ ADC
L'ADS1256 ha una non linearità (INL - Integral Non-Linearity) tipica di ±0.0003% FSR (Full Scale Range) e massima di ±0.0015% FSR.
Confrontando:
- Cella di carico: ±0.030% RO
- ADS1256: ±0.0015% FSR (worst case)
La non linearità dell'ADC è circa 20 volte migliore della cella di carico, quindi praticamente trascurabile nel budget di errore complessivo del sistema. Il fattore limitante è decisamente la cella di carico.

CALIBRAZIONE
•	la calibrazione ha due scopi principali:
•	Azzeramento (offset q):
•	Rimuove il peso della struttura
•	Compensa derive termiche
•	Idealmente pone lo zero a metà range ADC
•	Taratura scala (m):
•	Converte conteggi ADC in unità fisiche (kg, N)
•	Compensa differenze tra celle
•	Corregge non linearità del sistema
•	La posizione dello zero a metà range è una scelta progettuale che:
•	Massimizza range dinamico bidirezionale
