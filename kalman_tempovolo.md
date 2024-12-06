

# Derivazione Teorica del Filtro di Kalman per Altezza del Salto

## 1. Fisica del Salto

### Formula Base
```math
h = \frac{g \cdot t^2}{8}
```
Questa formula deriva da:
1. Moto uniformemente accelerato in caduta libera
2. Tempo di volo è simmetrico (salita = discesa)
3. g = accelerazione di gravità (9.81 m/s²)

### Variabili di Stato
```math
x = \begin{bmatrix} h \\ t \end{bmatrix}
```
dove:
- h: altezza del salto
- t: tempo di volo

## 2. Modello del Sistema

### Equazione di Transizione di Stato
```math
\begin{bmatrix} h_{k+1} \\ t_{k+1} \end{bmatrix} = 
\begin{bmatrix} 
1 & \frac{g\Delta t}{4} \\
0 & 1
\end{bmatrix}
\begin{bmatrix} h_k \\ t_k \end{bmatrix}
```

### Equazione di Misura
```math
z_k = \begin{bmatrix} 0 & 1 \end{bmatrix} \begin{bmatrix} h_k \\ t_k \end{bmatrix} + v_k
```
dove z_k è il tempo di volo misurato

## 3. Derivazione delle Matrici

### Matrice di Transizione F
```math
F = \begin{bmatrix} 
1 & \frac{g\Delta t}{4} \\
0 & 1
\end{bmatrix}
```
deriva da:
```math
h_{k+1} = h_k + \frac{g}{4}t_k\Delta t
```

### Matrice di Osservazione H
```math
H = \begin{bmatrix} 0 & 1 \end{bmatrix}
```
poiché misuriamo direttamente il tempo di volo

## 4. Rumore e Incertezze

### Rumore di Processo Q
```math
Q = \begin{bmatrix} 
0.01 & 0 \\
0 & 0.01
\end{bmatrix}
```
Rappresenta l'incertezza nel modello dinamico

### Rumore di Misura R
```math
R = [measurement\_noise]
```
Rappresenta l'incertezza nella misura del tempo

## 5. Equazioni del Filtro di Kalman

### Predizione
```math
\begin{align*}
\hat{x}_{k|k-1} &= F\hat{x}_{k-1|k-1} \\
P_{k|k-1} &= FP_{k-1|k-1}F^T + Q
\end{align*}
```

### Aggiornamento
```math
\begin{align*}
K_k &= P_{k|k-1}H^T(HP_{k|k-1}H^T + R)^{-1} \\
\hat{x}_{k|k} &= \hat{x}_{k|k-1} + K_k(z_k - H\hat{x}_{k|k-1}) \\
P_{k|k} &= (I - K_kH)P_{k|k-1}
\end{align*}
```

## 6. Considerazioni Implementative

1. **Rilevamento del Salto**
   - Soglia di stacco: 0.2 × peso_statico
   - Soglia di atterraggio: 1.5 × peso_statico

2. **Calcolo Tempo di Volo**
```math
t_{volo} = \frac{t_{atterraggio} - t_{stacco}}{f_{campionamento}}
```

3. **Stima Altezza**
```math
h_{stimata} = \frac{g \cdot t_{stimato}^2}{8}
```

Questo filtro è ottimizzato per:
- Robustezza al rumore nelle misure di tempo
- Minimizzazione dell'errore quadratico medio
- Stima in tempo reale dell'altezza

Vuoi approfondire qualche aspetto specifico della derivazione?
