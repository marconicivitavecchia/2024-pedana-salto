>[Torna all'indice](readme.md#fasi-progetto)

pippo: $ \dot{F}\text{pedana}(t) = \frac{dF\text{pedana}(t)}{dt} $

$$ {\dot{F}}_pedana(t) > \text{soglia}_\text{positivo} $$

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
