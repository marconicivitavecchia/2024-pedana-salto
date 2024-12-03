>[Torna all'indice](readme.md#fasi-progetto)

Ecco il codice Markdown che puoi copiare e incollare direttamente:

```markdown
# Metodi per Stabilire Soglie e Rilevare Eventi nei Salti

Questi metodi sono utili per rilevare automaticamente gli eventi chiave di un salto, come:
- **Inizio salto**: Quando l'atleta inizia ad esercitare forza sulla pedana.
- **Stacco**: Momento in cui l'atleta lascia completamente la pedana.
- **Attacco alla pedana**: Quando l'atleta riatterra sulla pedana.
- **Fine caduta**: Stabilizzazione finale del segnale.

---

## **1. Metodo della Derivata del Segnale**
Utilizza la derivata del segnale della forza per rilevare le transizioni chiave.

### **Descrizione**
1. Calcola la derivata del segnale:  
   `dF_pedana(t)/dt`
2. Rileva le soglie in base a variazioni significative:
   - **Inizio salto**: Quando `dF_pedana(t)/dt > soglia_positivo`.
   - **Stacco**: Quando `F_pedana(t) ≈ 0`.
   - **Attacco alla pedana**: Picco negativo significativo di `dF_pedana(t)/dt`.
   - **Fine caduta**: Stabilizzazione di `F_pedana(t)` intorno al peso iniziale.

### **Vantaggi**
- Alta precisione per transizioni rapide.
- Adatto a segnali di alta qualità e movimenti esplosivi.

### **Svantaggi**
- Sensibile al rumore, richiede filtraggio del segnale.

---

## **2. Metodo Statistico (Soglie Fisse)**
Rileva gli eventi basandosi su soglie fisse definite rispetto al segnale a riposo.

### **Descrizione**
1. Calibra la pedana per ottenere il valore medio del segnale a riposo:  
   `F_pedana, riposo`
2. Imposta soglie relative:  
   - `soglia_positivo`: Transizioni verso l’alto.  
   - `soglia_negativo`: Transizioni verso il basso.
3. Rileva gli eventi:
   - **Inizio salto**: Quando `F_pedana(t) > F_pedana, riposo + soglia_positivo`.
   - **Stacco**: Quando `F_pedana(t) < soglia_negativo`.
   - **Attacco alla pedana**: Quando `F_pedana(t) > F_pedana, riposo + soglia_positivo` dopo uno zero-crossing.
   - **Fine caduta**: Stabilizzazione del segnale intorno a `F_pedana, riposo`.

### **Vantaggi**
- Semplice da implementare.
- Robusto in presenza di segnali stabili.

### **Svantaggi**
- Meno preciso per salti complessi o segnali rumorosi.

---

## **3. Metodo del Filtro Adattivo (Moving Average & Outlier Detection)**
Utilizza un filtro adattivo per rilevare transizioni significative rispetto alla media dinamica.

### **Descrizione**
1. Calcola la media mobile del segnale:  
   `Media_n = (1/w) * Σ_{i=n-w}^{n} F_pedana[i]`  
   Dove `w` è la finestra temporale (es. 50 ms).
2. Calcola le deviazioni dal segnale:  
   `Delta_F = F_pedana(t) - Media_n`
3. Rileva gli eventi:
   - **Inizio salto**: Quando `Delta_F > soglia_positivo`.
   - **Stacco**: Quando `Delta_F < -soglia_negativo`.
   - **Attacco alla pedana**: Picco positivo dopo uno zero-crossing.
   - **Fine caduta**: Stabilizzazione di `Delta_F` intorno a zero.

### **Vantaggi**
- Adattivo, robusto contro variazioni lente e graduali.

### **Svantaggi**
- Richiede configurazione delle soglie.

---

## **4. Metodo dell’Energia del Segnale**
Analizza l’energia del segnale per rilevare le transizioni.

### **Descrizione**
1. Calcola l’energia istantanea del segnale:  
   `E(t) = F_pedana(t)^2`
2. Rileva gli eventi analizzando i picchi:
   - **Inizio salto**: Picco positivo di `dE(t)/dt`.
   - **Stacco**: Minimo locale di `E(t)` vicino a zero.
   - **Attacco alla pedana**: Picco positivo dopo il minimo.
   - **Fine caduta**: Stabilizzazione di `E(t)`.

### **Vantaggi**
- Facile da implementare.
- Robusto contro rumore ad alta frequenza.

### **Svantaggi**
- Sensibile a picchi accidentali.

---

## **5. Metodo del Machine Learning**
Utilizza modelli predittivi per identificare automaticamente i momenti chiave.

### **Descrizione**
1. **Preparazione**:
   - Raccogli dati di salti e annota manualmente gli eventi (`t_inizio`, `t_stacco`, `t_attacco`, `t_fine`).
2. **Feature Extraction**:
   - Derivata del segnale, varianza, deviazioni dalla media, etc.
3. **Allenamento**:
   - Usa classificatori (es. SVM, Random Forest) o modelli sequenziali (es. LSTM).
4. **Predizione**:
   - Il modello analizza il segnale in tempo reale e rileva gli eventi.

### **Vantaggi**
- Adatto a movimenti complessi e segnali rumorosi.
- Può generalizzare bene su dataset vari.

### **Svantaggi**
- Richiede un dataset per l'addestramento.
- Complessità implementativa maggiore.

---

## **Confronto dei Metodi**

| **Metodo**                     | **Vantaggi**                                        | **Svantaggi**                                 |
|--------------------------------|----------------------------------------------------|----------------------------------------------|
| **Derivata del segnale**       | Alta precisione per cambiamenti rapidi             | Sensibile al rumore                          |
| **Statistico (soglie fisse)**  | Semplice da implementare                           | Poco preciso per segnali rumorosi            |
| **Filtro adattivo**            | Robusto contro variazioni lente e graduali         | Richiede configurazione delle soglie         |
| **Energia del segnale**        | Facile da implementare e robusto                  | Sensibile a picchi accidentali               |
| **Machine Learning**           | Adattabile a movimenti complessi e dataset vari    | Richiede dataset e competenze avanzate       |

---

## **Conclusioni**
- Per salti semplici e segnali stabili: **Metodo statistico** o **derivata del segnale**.
- Per segnali rumorosi o variabili: **Filtro adattivo** o **energia del segnale**.
- Per analisi avanzate: **Machine Learning**.

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
