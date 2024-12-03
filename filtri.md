>[Torna all'indice](readme.md#fasi-progetto)
>
# Filtri per Motion Tracking

## Filtro EMA (Exponential Moving Average)

### Formula e Parametri

```python
y[n] = α * x[n] + (1-α) * y[n-1]
```

Dove:
- `x[n]` è il valore misurato corrente
- `y[n]` è il valore filtrato  
- `y[n-1]` è il valore filtrato precedente
- `α` è il fattore di smoothing (0 < α < 1)

### Proprietà Chiave

Il filtro EMA presenta le seguenti caratteristiche principali:

- Reattività: un α alto (tendente a 1) rende il filtro più reattivo ai cambiamenti
- Smoothing: un α basso (tendente a 0) produce un segnale più smussato
- Latenza: minima, rendendolo ideale per tracking in tempo reale

### Vantaggi nel Motion Tracking

Il filtro EMA offre diversi benefici:

- Ottimizza lo smoothing delle piccole oscillazioni nella velocità
- Mantiene un'eccellente reattività per i movimenti rapidi
- Richiede risorse computazionali minime

## Filtro di Kalman

### Equazioni Fondamentali

Fase di predizione:

```python
x̂ₖ|ₖ₋₁ = Fₖx̂ₖ₋₁|ₖ₋₁
Pₖ|ₖ₋₁ = FₖPₖ₋₁|ₖ₋₁Fₖᵀ + Qₖ
```

Fase di aggiornamento:

```python
Kₖ = Pₖ|ₖ₋₁Hₖᵀ(HₖPₖ|ₖ₋₁Hₖᵀ + Rₖ)⁻¹
x̂ₖ|ₖ = x̂ₖ|ₖ₋₁ + Kₖ(zₖ - Hₖx̂ₖ|ₖ₋₁)
Pₖ|ₖ = (I - KₖHₖ)Pₖ|ₖ₋₁
```

Parametri principali:
- x̂: stima dello stato
- P: matrice di covarianza dell'errore
- K: guadagno di Kalman
- Q: rumore del processo
- R: rumore della misura

### Caratteristiche Distintive

Il filtro di Kalman si caratterizza per:

- Ottimalità nella minimizzazione dell'errore quadratico medio
- Capacità di adattarsi dinamicamente al livello di rumore
- Incorporazione di un modello predittivo del sistema

### Applicazioni nel Motion Tracking

Il filtro risulta particolarmente efficace per:

- Filtraggio avanzato di posizioni rumorose
- Gestione ottimale delle discontinuità nelle misure
- Fornire stime affidabili dell'incertezza

## Filtro Complementare

### Implementazione Matematica

Calcolo della direzione:

```python
dx_dir = (x - x_last) / √[(x - x_last)² + (y - y_last)² + ε]
dy_dir = (y - y_last) / √[(x - x_last)² + (y - y_last)² + ε]
```

Stima della posizione basata sulla velocità:

```python
x_from_v = x_last + v * Δt * dx_dir
y_from_v = y_last + v * Δt * dy_dir
```

Fusione finale dei dati:

```python
x_filtered = α * x + (1-α) * x_from_v
y_filtered = α * y + (1-α) * y_from_v
```

### Caratteristiche Principali

Il filtro complementare si distingue per:

- Capacità di fusione tra misure dirette e derivate
- Sfruttamento ottimale dei punti di forza di diverse misure
- Bilanciamento efficace tra rumore ad alta e bassa frequenza

### Vantaggi nel Motion Tracking

Offre benefici significativi:

- Fusione ottimale tra dati di posizione e velocità
- Miglioramento della continuità del movimento
- Riduzione efficace di drift e rumore

## Comparazione delle Performance

### Movimenti Rapidi
- EMA: Eccellente per il tracking della velocità
- Kalman: Può introdurre un eccessivo smoothing
- Complementare: Ottimo con dati di velocità disponibili

### Movimenti Lenti
- EMA: Può presentare ritardi
- Kalman: Fornisce un filtraggio eccellente
- Complementare: Offre stabilità superiore

### Gestione del Rumore
- EMA: Performance limitate
- Kalman: Filtraggio ottimale
- Complementare: Rappresenta un buon compromesso

### Risorse Computazionali
- EMA: Requisiti minimi
- Kalman: Richiede risorse significative
- Complementare: Necessità moderate

>[Torna all'indice](readme.md#fasi-progetto)
