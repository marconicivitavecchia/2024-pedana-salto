>[Torna all'indice](readme.md#fasi-progetto)>[Torna a filtri](filtri.md)

# Filtri Avanzati per Motion Tracking

## Filtro di Madgwick

### Proprietà

Il filtro di Madgwick è specializzato per:

- Fusione dati da IMU (accelerometro, giroscopio)
- Efficienza computazionale
- Gestione del gimbal lock
- Orientamento 3D

### Formula Principale

```
q̇ₑₛₜ = q̇ω - β∇f
```

Parametri:
- q̇ₑₛₜ: derivata del quaternione stimato
- β: gain del gradiente discendente
- ∇f: gradiente della funzione obiettivo

## Filtro a Media Mobile Ponderata

### Formula Base

```
y[n] = (w₁x[n] + w₂x[n-1] + ... + wₖx[n-k+1]) / (w₁ + w₂ + ... + wₖ)
```

### Vantaggi

- Maggiore flessibilità rispetto alla media mobile semplice
- Ponderazione personalizzabile dei campioni recenti
- Ottimo bilanciamento tra smoothing e preservazione dei dettagli

## Filtro di Savitzky-Golay

### Caratteristiche

- Preservazione dei momenti di ordine alto (picchi)
- Riduzione del rumore con mantenimento della forma
- Ottimizzato per analisi di accelerazione e jerk

### Formula Base

```
y[n] = Σᵢ cᵢx[n+i]
```

Dove cᵢ sono i coefficienti del polinomio di fitting

## Filtro di Butterworth

### Caratteristiche Principali

- Risposta in frequenza massimamente piatta
- Ordine configurabile per diversi livelli di filtraggio
- Ideale per pre-processamento

### Formula (Dominio s)

```
|H(jω)|² = 1 / (1 + (ω/ωc)^(2N))
```

Parametri:
- ωc: frequenza di taglio
- N: ordine del filtro

## Particle Filter

### Vantaggi Chiave

- Gestione di distribuzioni non gaussiane
- Tracking multi-target
- Robustezza alle occlusioni

### Algoritmo Base

1. Predizione particelle
2. Update pesi
3. Resampling
4. Stima stato

## Filtro H∞

### Caratteristiche

- Robustezza alle incertezze di modello
- Ottimizzato per sistemi non lineari
- Minimizzazione dell'errore nel caso peggiore

### Formula Base

```
K∞ = PH^T(I - θYP + HR⁻¹H^T P)⁻¹R⁻¹
```

## Analisi Comparativa

### Movimenti Complessi

- **Madgwick**: Eccellente per orientamento 3D
- **Savitzky-Golay**: Ottimo per analisi dettagliata
- **Particle**: Ideale per tracking multi-target

### Rumore Non Gaussiano

- **H∞**: Molto robusto
- **Particle**: Gestisce distribuzioni arbitrarie
- **Weighted MA**: Buon compromesso complessità/prestazioni

### Performance Real-time

- **Madgwick**: Efficiente e veloce
- **Weighted MA**: Bassa latenza
- **Butterworth**: Implementazione efficiente

### Precisione vs Complessità

- **Particle**: Alta precisione, alta complessità
- **Savitzky-Golay**: Buona precisione, complessità media
- **Weighted MA**: Precisione moderata, bassa complessità

>[Torna all'indice](readme.md#fasi-progetto)>[Torna a filtri](filtri.md)
