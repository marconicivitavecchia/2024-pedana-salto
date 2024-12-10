>[Torna all'indice](readme.md#fasi-progetto)



Ecco il contenuto tradotto in Markdown per GitHub, seguendo le istruzioni che hai fornito:

## **1. Metodo della derivata del segnale**

Questo metodo si basa sul calcolo della variazione temporale della forza (F_pedana(t)) per rilevare cambiamenti improvvisi (indicativi di stacco o attacco).

**Fasi**

1. **Derivata temporale**:
   * Calcola la derivata del segnale: $$ \dot{F}(t) = \frac{F_{\text{pedana}}(t_{i+1}) - F_{\text{pedana}}(t_i)}{\Delta t} $$
   * Nel dominio discreto: $\dot{F}[n] = \frac{F_{\text{pedana}}[n+1] - F_{\text{pedana}}[n]}{\Delta t}$

2. **Soglie**:
   * **Inizio salto**: Identifica il primo punto in cui $\dot{F}(t) > \text{soglia}_\text{derivata}$. Questo indica un aumento rapido della forza (pre-caricamento del salto).
   * **Stacco**: Trova il momento in cui $F_{\text{pedana}}(t)$ scende sotto una soglia minima (es. 5% del peso statico), segnalando che l'atleta ha lasciato la pedana.
   * **Attacco alla pedana (caduta)**: Identifica il primo punto in cui $F_{\text{pedana}}(t)$ aumenta rapidamente sopra il 5% del peso statico.
   * **Fine caduta**: Quando la derivata torna a valori vicini a zero, segnalando che la forza si è stabilizzata.

3. **Calibrazione delle soglie**:
   * $soglia_derivata$: Calcolata in base al rumore del segnale (ad esempio, 3σ del rumore di fondo).
   * 5% del peso statico come riferimento pratico per rilevare stacco e attacco.


## **2. Metodo statistico sul segnale grezzo**

Questo approccio utilizza analisi statistica e confronto con il valore medio della forza a riposo (F_statico).

**Fasi**

1. **Calcolo del peso statico**:
   * Media della forza F_statico durante una finestra temporale di riferimento, prima del salto o della caduta. $F_{\text{statico}} = \frac{1}{N} \sum_{n=1}^{N} F_{\text{pedana}}[n]$

2. **Definizione delle soglie**:
   * **Inizio salto**: Identifica il primo istante in cui $F_{\text{pedana}}[n] > F_{\text{statico}} + \text{soglia}_\text{statica}$. La soglia può essere un valore fisso (ad esempio, 10% di F_statico).
   * **Stacco**: Trova il punto in cui $F_{\text{pedana}}[n] < \text{soglia}_\text{bassa}$, con $\text{soglia}_\text{bassa} \approx 5\%$ di F_statico.
   * **Attacco alla pedana**: Quando $F_{\text{pedana}}[n] > \text{soglia}_\text{bassa}$ durante la caduta.
   * **Fine caduta**: Identifica il momento in cui $F_{\text{pedana}}[n]$ si stabilizza nuovamente intorno a F_statico.

3. **Filtraggio del segnale**:
   * Applica un filtro passa-basso (ad esempio, filtro di media mobile) per ridurre il rumore e migliorare l'affidabilità delle soglie.

## **3. Metodo del filtro adattivo (Moving Average & Outlier Detection)**

Questo metodo utilizza un filtro adattivo per rilevare transizioni significative nel segnale basandosi su variazioni rispetto alla media dinamica.

**Fasi**

1. **Calcolo della media mobile**:
   * Applica un filtro di media mobile su F_pedana(t): $$\text{Median} = \frac{1}{w} \sum_{i=n-w}^n F_{\text{pedana}}[i]$$ Dove w è la finestra di media mobile (es. 50 ms).
  
2. **Calcolo delle deviazioni**:
   * Determina la differenza tra il segnale F_pedana(t) e la media mobile: $\Delta F = F_{\text{pedana}}(t) - \text{Media}_n$

3. **Rilevamento degli eventi**:
   * **Inizio salto**: Identifica il primo punto in cui $\Delta F > \text{soglia}_\text{positivo}$.
   * **Stacco**: Quando $\Delta F < -\text{soglia}_\text{negativo}$.
   * **Attacco alla pedana**: Quando $\Delta F > \text{soglia}_\text{positivo}$ dopo uno zero-crossing.
   * **Fine caduta**: Quando $\Delta F$ torna vicino a zero (stabilizzazione).

$$ \text{soglia}_\text{negativo} $$, $$ \text{soglia}_\text{negativo} $$ 

4. **Calibrazione delle soglie**:
   * **Le soglie** $\text{soglia}_\text{negativo} e \text{soglia}_\text{negativo}$ vengono definite come $n\sigma$ (ad esempio, $n=3$) della deviazione standard del segnale a riposo.

**Vantaggi**
* Robusto contro il rumore.
* Adatto a segnali non lineari o con variazioni lente.


## **4. Metodo basato su Machine Learning**

Questo metodo utilizza modelli di classificazione o segmentazione del segnale per identificare automaticamente i momenti chiave.

**Fasi**

1. **Preparazione dei dati**:
   * Campiona il segnale F_pedana(t) durante salti noti e annota manualmente gli eventi (t_inizio, t_stacco, t_attacco, t_fine).

2. **Feature Extraction**:
   * Calcola caratteristiche come:
      * Derivata (Ḟ).
      * Deviazione dalla media.
      * Varianza su finestre mobili.
      * Peak detection (massimi locali).

3. **Allenamento del Modello**:
   * Usa un classificatore (es. SVM, Random Forest) o un modello sequenziale (es. LSTM) per identificare gli eventi in base ai pattern del segnale.

4. **Predizione**:
   * Il modello analizza il segnale in tempo reale e assegna etichette temporali agli eventi.

**Vantaggi**
* Adattabile a diversi tipi di segnali.
* Riconosce automaticamente eventi complessi.
* Utilizzabile su pedane con variazioni individuali.

**Svantaggi**
* Richiede un dataset per l'addestramento.
* Più complesso da implementare rispetto ai metodi tradizionali.


## **5. Metodo dell'energia (Signal Energy Analysis)**

Questo metodo analizza l'energia del segnale per identificare transizioni significative.

**Fasi**

1. **Calcolo dell'energia istantanea**:
   * Energia istantanea del segnale: E(t) = F_pedana(t)^2

2. **Analisi delle variazioni**:
   * L'inizio salto e l'attacco alla pedana corrispondono a aumenti bruschi di energia:
      * **Inizio salto**: Picco positivo di Ė(t).
      * **Stacco**: Minimo locale di E(t) (vicino a zero).
      * **Attacco alla pedana**: Picco positivo dopo il minimo.
      * **Fine caduta**: Stabilizzazione di E(t) a un valore costante.

3. **Calibrazione delle soglie**:
   * Determina soglie per identificare picchi e minimi basandoti su dati a riposo.

**Vantaggi**
* Facile da implementare.
* Robusto contro rumore ad alta frequenza. 
* Ideale per movimenti esplosivi (es. salti atletici).


**Confronto tra i metodi**

**Metodo**|**Vantaggi**|**Svantaggi**
---|---|---
Derivata del segnale|Preciso per transizioni rapide|Sensibile al rumore
Statistico|Robusto al rumore, semplice|Meno adatto a movimenti complessi  
Filtro adattivo|Migliora la stabilità su segnali variabili|Richiede regolazione fine delle soglie
Machine Learning|Adattabile a segnali complessi|Richiede dataset e competenze avanzate
Energia del segnale|Facile da implementare, intuitivo|Sensibile a picchi accidentali

Ecco un breve confronto tra i diversi metodi per l'analisi del segnale della pedana:

- Il **metodo della derivata del segnale** è preciso nel rilevare transizioni rapide, ma è sensibile al rumore.
- Il **metodo statistico** è robusto al rumore e semplice da implementare, ma meno adatto per movimenti complessi.
- Il **metodo del filtro adattivo** migliora la stabilità su segnali con variazioni lente, ma richiede una fine regolazione delle soglie.
- Il **metodo basato su machine learning** è adattabile a segnali complessi, ma richiede un dataset per l'addestramento e competenze avanzate.
- Il **metodo dell'energia del segnale** è facile da implementare e intuitivo, ma può essere sensibile a picchi accidentali.

La scelta del metodo dipende dalle caratteristiche del segnale e dalle esigenze specifiche dell'applicazione. Per movimenti semplici e ben definiti, i metodi statistico o della derivata possono essere sufficienti. Per segnali più complessi o rumorosi, i metodi del filtro adattivo o dell'energia potrebbero essere più adatti. Infine, per analisi avanzate o situazioni personalizzate, il metodo basato su machine learning potrebbe essere la scelta migliore, anche se richiede risorse aggiuntive.

Ciascun metodo implementa una strategia diversa per rilevare gli eventi chiave del salto:

- **Derivata:** usa il tasso di cambio della forza
- **Statistico:** usa soglie basate su media e deviazione standard
- **Filtro adattivo:** confronta con media mobile
- **Energia:** analizza il quadrato della forza
- **Machine Learning:** usa features estratte da finestre temporali

Il codice include una funzione per generare dati di test realistici e mostra come applicare ciascun metodo.

```python
import numpy as np
from scipy.signal import savgol_filter
from sklearn.ensemble import RandomForestClassifier

def generate_sample_data():
    # Crea array di tempo: 200 punti in 2 secondi (100Hz)
    t = np.linspace(0, 2, 200)  
    static_weight = 700  # Peso statico simulato in Newton
    # Aggiunge rumore gaussiano con media 0 e deviazione standard 10
    noise = np.random.normal(0, 10, len(t))
    
    # Inizializza array forza con peso statico
    force = static_weight * np.ones_like(t)
    
    # Simula le fasi del salto:
    # Fase di caricamento: rampa lineare che aumenta la forza
    force[40:60] += 300 * (t[40:60] - t[40])
    # Fase di volo: forza zero mentre atleta è in aria
    force[60:80] = 0
    # Fase di atterraggio: picco esponenziale decrescente
    force[80:100] = static_weight + 500 * np.exp(-(t[80:100] - t[80])*10)
    
    return t, force + noise

def derivative_method(t, force, threshold=1000):
    # Calcola derivata come differenza tra punti consecutivi diviso dt
    derivative = np.diff(force) / np.diff(t)
    events = {}
    
    # Trova inizio salto quando derivata supera soglia
    events['start'] = np.where(derivative > threshold)[0][0]
    
    # Trova stacco quando forza scende sotto 5% del peso statico
    events['takeoff'] = np.where(force < 0.05 * force[:20].mean())[0][0]
    
    # Trova atterraggio quando forza torna sopra 5% del peso statico
    landing_idx = np.where(force[events['takeoff']:] > 0.05 * force[:20].mean())[0][0]
    events['landing'] = events['takeoff'] + landing_idx
    
    return events

def statistical_method(force, static_weight):
    # Calcola media (peso statico) e deviazione standard del rumore
    mean = static_weight
    std = np.std(force[:100])
    
    events = {}
    # Inizio: forza supera media + 3 deviazioni standard
    events['start'] = np.where(force > mean + 3*std)[0][0]
    # Stacco: forza sotto 5% del peso statico
    events['takeoff'] = np.where(force < 0.05 * mean)[0][0]
    # Atterraggio: forza torna sopra 50% del peso statico
    events['landing'] = np.where(force[events['takeoff']:] > 0.5 * mean)[0][0] + events['takeoff']
    
    return events

def adaptive_filter_method(force, window=20):
    # Applica media mobile: per ogni punto, media dei 'window' punti precedenti
    moving_avg = np.convolve(force, np.ones(window)/window, mode='valid')
    # Calcola differenza tra segnale originale e media mobile
    delta = force[window-1:] - moving_avg
    
    events = {}
    # Trova eventi usando deviazioni dalla media mobile
    events['start'] = np.where(delta > np.std(delta)*3)[0][0]
    events['takeoff'] = np.where(force < 0.05 * force[:20].mean())[0][0]
    events['landing'] = np.where(force[events['takeoff']:] > 0.05 * force[:20].mean())[0][0] + events['takeoff']
    
    return events

def energy_method(force):
    # Calcola energia come quadrato della forza
    energy = force**2
    # Derivata dell'energia per trovare variazioni rapide
    energy_derivative = np.diff(energy)
    
    events = {}
    # Trova eventi usando variazioni di energia
    events['start'] = np.where(energy_derivative > np.std(energy_derivative)*3)[0][0]
    events['takeoff'] = np.where(energy < 0.1 * energy[:20].mean())[0][0]
    events['landing'] = np.where(energy[events['takeoff']:] > energy[:20].mean())[0][0] + events['takeoff']
    
    return events

def ml_method(force, train=True):
    window = 20
    # Crea matrice di features: per ogni punto, 3 caratteristiche calcolate su finestra mobile
    features = np.zeros((len(force)-window, 3))
    for i in range(len(force)-window):
        window_data = force[i:i+window]
        features[i] = [
            np.mean(window_data),      # Media nella finestra
            np.std(window_data),       # Deviazione standard nella finestra
            np.max(window_data) - np.min(window_data)  # Range nella finestra
        ]
    
    if train:
        # Crea labels simulate per training (0=statico, 1=caricamento, 2=volo, 3=atterraggio)
        labels = np.zeros(len(features))
        labels[40:60] = 1  # Fase di caricamento
        labels[60:80] = 2  # Fase di volo
        labels[80:100] = 3 # Fase di atterraggio
        
        # Allena Random Forest con 10 alberi decisionali
        clf = RandomForestClassifier(n_estimators=10)
        clf.fit(features, labels)
        return clf
    
    return features

# Test dei metodi
if __name__ == "__main__":
    # Genera dati di test
    t, force = generate_sample_data()
    
    # Applica tutti i metodi
    events_derivative = derivative_method(t, force)
    events_statistical = statistical_method(force, force[:20].mean())
    events_adaptive = adaptive_filter_method(force)
    events_energy = energy_method(force)
    
    # Applica metodo ML
    clf = ml_method(force, train=True)
    features = ml_method(force, train=False)
    predictions = clf.predict(features)
    
    # Stampa risultati
    print("Eventi rilevati:")
    print("Metodo Derivata:", events_derivative)
    print("Metodo Statistico:", events_statistical)
    print("Metodo Filtro Adattivo:", events_adaptive)
    print("Metodo Energia:", events_energy)
    print("ML classi uniche trovate:", np.unique(predictions))
```

Ciascun metodo implementa una strategia diversa per rilevare gli eventi chiave del salto:
1. Derivata: usa il tasso di cambio della forza
2. Statistico: usa soglie basate su media e deviazione standard
3. Filtro adattivo: confronta con media mobile
4. Energia: analizza il quadrato della forza
5. Machine Learning: usa features estratte da finestre temporali

Il codice include una funzione per generare dati di test realistici e mostra come applicare ciascun metodo.

>[Torna all'indice](readme.md#fasi-progetto)
