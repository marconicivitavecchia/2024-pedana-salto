>[Torna all'indice](readme.md#fasi-progetto)

# Metodi di Rilevamento per Analisi del Salto

## 1. Metodo della Derivata del Segnale

### Calcolo Derivata
$$\dot{F}(t) = \frac{F_{\text{pedana}}(t_{i+1}) - F_{\text{pedana}}(t_i)}{\Delta t}$$

Nel dominio discreto:
$$\dot{F}[n] = \frac{F_{\text{pedana}}[n+1] - F_{\text{pedana}}[n]}{\Delta t}$$

### Soglie
- **Inizio salto**: $\dot{F}(t) > \text{soglia}_{\text{derivata}}$
- **Stacco**: $F_{\text{pedana}}(t) < 5\% \text{ del peso statico}$
- **Attacco**: $F_{\text{pedana}}(t)$ aumenta sopra 5% del peso statico
- **Fine caduta**: $\dot{F}(t) \approx 0$

## 2. Metodo Statistico

### Calcolo Peso Statico
$$F_{\text{statico}} = \frac{1}{N} \sum_{n=1}^N F_{\text{pedana}}[n]$$

### Definizione Eventi
- **Inizio salto**: $F_{\text{pedana}}[n] > F_{\text{statico}} + \text{soglia}_{\text{statica}}$
- **Stacco**: $F_{\text{pedana}}[n] < \text{soglia}_{\text{bassa}}$ ($\approx 5\%$ di $F_{\text{statico}}$)
- **Attacco**: $F_{\text{pedana}}[n] > \text{soglia}_{\text{bassa}}$
- **Fine caduta**: $F_{\text{pedana}}[n] \approx F_{\text{statico}}$

## 3. Metodo del Filtro Adattivo

### Media Mobile
$$\text{Media}_n = \frac{1}{w} \sum_{i=n-w}^n F_{\text{pedana}}[i]$$

### Calcolo Deviazioni
$$\Delta F = F_{\text{pedana}}(t) - \text{Media}_n$$

### Eventi
- **Inizio salto**: $\Delta F > \text{soglia}_{\text{positivo}}$
- **Stacco**: $\Delta F < -\text{soglia}_{\text{negativo}}$
- **Attacco**: $\Delta F > \text{soglia}_{\text{positivo}}$ dopo zero-crossing
- **Fine caduta**: $\Delta F \approx 0$

## 4. Metodo dell'Energia

### Energia Istantanea
$$E(t) = F_{\text{pedana}}(t)^2$$

### Eventi
- **Inizio salto**: Picco positivo di $\dot{E}(t)$
- **Stacco**: Minimo locale di $E(t)$
- **Attacco**: Picco positivo dopo minimo
- **Fine caduta**: $E(t)$ stabilizzata

# 5. Metodo Machine Learning

## Applicazione
- Dataset significativo di salti esistenti
- Analisi avanzata con modelli predittivi

## Feature Extraction
$$\text{Features} = \{F_{\text{pedana}}(t), \dot{F}(t), \sigma_F, \text{peaks}, \text{crossings}\}$$

## Eventi da Classificare
- $t_{\text{inizio}}$: Inizio salto
- $t_{\text{stacco}}$: Stacco dalla pedana
- $t_{\text{attacco}}$: Contatto con pedana
- $t_{\text{fine}}$: Fine movimento

## Caratteristiche
### Pro
- Adattabile a scenari diversi
- Riconosce pattern complessi

### Contro
- Training iniziale necessario
- Alto costo computazionale

## Uso Ottimale
Studi di ricerca e sport di alto livello con requisiti di analisi personalizzata

## Confronto Metodi

| Metodo | Vantaggi | Svantaggi |
|--------|----------|-----------|
| Derivata | Preciso per transizioni rapide | Sensibile al rumore |
| Statistico | Robusto, semplice | Meno adatto a movimenti complessi |
| Filtro adattivo | Migliore stabilità | Richiede regolazione fine |
| Energia | Facile implementazione | Sensibile a picchi accidentali |
| Machine Learning | Adattabile a scenari diversi, riconosce pattern complessi | Alto costo computazionale, richiede training |

## Esempi in python

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
