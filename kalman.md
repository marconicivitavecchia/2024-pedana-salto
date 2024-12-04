>[Torna all'indice](readme.md#fasi-progetto)>[Torna a filtri](filtri.md)
>
# Filtro di Kalman: Dal Concetto alla Teoria

## Livello 1: L'Idea Base

Il concetto fondamentale riguarda la stima della posizione di un oggetto in movimento, dove:

- Le misure dei sensori contengono rumore
- Si ha un modello approssimato del movimento
- È necessario combinare le informazioni in modo ottimale

### Il Ruolo del Filtro

Il filtro di Kalman agisce come un arbitro che:

1. Effettua una predizione della posizione
2. Confronta la predizione con le misure
3. Valuta l'affidabilità delle diverse fonti
4. Produce una stima ottimizzata

## Livello 2: Meccanismo di Funzionamento

### Ciclo Base

1. **Fase di Predizione**
   - Utilizza il modello del sistema per la predizione dello stato
   - Incrementa l'incertezza associata alla predizione

2. **Fase di Correzione**
   - Confronta predizione e misura
   - Valuta l'affidabilità delle fonti
   - Aggiorna la stima corrente

### Parametri Fondamentali

- Q: affidabilità del modello
- R: affidabilità delle misure
- K: peso della correzione
- P: incertezza della stima

## Livello 3: Fondamenti Matematici

### Equazioni di Predizione

```
x̂ₖ|ₖ₋₁ = Fₖx̂ₖ₋₁|ₖ₋₁
Pₖ|ₖ₋₁ = FₖPₖ₋₁|ₖ₋₁Fₖᵀ + Qₖ
```

### Equazioni di Correzione

```
Kₖ = Pₖ|ₖ₋₁Hₖᵀ(HₖPₖ|ₖ₋₁Hₖᵀ + Rₖ)⁻¹
x̂ₖ|ₖ = x̂ₖ|ₖ₋₁ + Kₖ(zₖ - Hₖx̂ₖ|ₖ₋₁)
Pₖ|ₖ = (I - KₖHₖ)Pₖ|ₖ₋₁
```

## Livello 4: Approfondimento Teorico

### Ipotesi di Base

1. Sistema lineare:
```
xₖ = Fₖxₖ₋₁ + wₖ₋₁
zₖ = Hₖxₖ + vₖ
```

2. Rumori gaussiani:
```
w ~ N(0,Q)
v ~ N(0,R)
```

### Proprietà di Ottimalità

Il filtro di Kalman è caratterizzato da:

1. Assenza di bias: E[x̂ₖ - xₖ] = 0
2. Minimizzazione della traccia di P
3. Ottimalità MMSE per rumori gaussiani

## Livello 5: Considerazioni Avanzate

### Gestione Non Linearità

Per sistemi non lineari sono disponibili varianti:

- EKF: basato su linearizzazione locale
- UKF: utilizza trasformazione unscented
- Particle Filter: approccio Monte Carlo

### Analisi di Stabilità

La convergenza dipende da:

1. Osservabilità del sistema
2. Controllabilità del sistema
3. Inizializzazione corretta di P

### Aspetti Implementativi

Punti critici:

1. Mantenimento della simmetria di P
2. Garanzia della positività definita
3. Gestione del condizionamento numerico

## Guida ai Livelli di Comprensione

- **Livello 1**: Comprensione intuitiva
- **Livello 2**: Capacità implementativa
- **Livello 3**: Competenze di ottimizzazione
- **Livello 4**: Comprensione dei limiti
- **Livello 5**: Capacità di estensione

Sitografia:
- https://www.dmi.unict.it/santoro/teaching/psr/slides/KalmanFiltering.pdf
- https://moodle2.units.it/course/section.php?id=116470

>[Torna all'indice](readme.md#fasi-progetto)>[Torna a filtri](filtri.md)
