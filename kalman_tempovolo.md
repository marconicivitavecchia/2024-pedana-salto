>[Torna all'indice](readme.md#fasi-progetto)

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

### In python  

```python
import numpy as np

class KalmanJumpHeight:
    def __init__(self, dt, measurement_noise=0.1):
        # Stato: [altezza, tempo_volo]
        self.x = np.array([0., 0.])
        self.g = 9.81
        self.dt = dt
        
        # Matrice di transizione 
        self.F = np.array([
            [1, self.g*dt/4],  # h = g*t²/8
            [0, 1]             # tempo volo
        ])
        
        # Matrice di misura (misuriamo il tempo)
        self.H = np.array([[0, 1]])
        
        # Covarianze
        self.P = np.eye(2)
        self.Q = np.array([[0.01, 0],
                          [0, 0.01]])  # Rumore processo
        self.R = np.array([[measurement_noise]])  # Rumore misura tempo
        
    def predict(self):
        self.x = self.F @ self.x
        self.P = self.F @ self.P @ self.F.T + self.Q
        
    def update(self, t_volo):
        # Innovazione
        y = t_volo - self.H @ self.x
        
        # Guadagno Kalman
        S = self.H @ self.P @ self.H.T + self.R
        K = self.P @ self.H.T @ np.linalg.inv(S)
        
        # Aggiornamento
        self.x = self.x + K @ [y]
        self.P = (np.eye(2) - K @ self.H) @ self.P
        
        # Calcolo altezza
        return (self.g * self.x[1]**2) / 8
```

#### Uso in Python
 
```python
def calcola_salto(forze, freq_camp, peso_statico):
    dt = 1/freq_camp
    kf = KalmanJumpHeight(dt)
    
    # Trova tempo di volo
    t_stacco = next(i for i, f in enumerate(forze) 
                    if f < peso_statico * 0.2)
    t_atterr = next(i for i, f in enumerate(forze[t_stacco:]) 
                    if f > peso_statico * 1.5) + t_stacco
    
    t_volo = (t_atterr - t_stacco) / freq_camp
    
    # Stima con Kalman
    kf.predict()
    altezza = kf.update(t_volo)
    
    return altezza, t_volo
```

### In micropython

```python
class KalmanJumpHeightMicro:
   def __init__(self, dt, measurement_noise=0.1):
       # Stato: [altezza, tempo_volo]
       self.x = [0.0, 0.0]
       self.g = 9.81
       self.dt = dt
       
       # Matrice di transizione
       self.F = [
           [1, self.g*dt/4],  # h = g*t²/8 
           [0, 1]             # tempo volo
       ]
       
       # Matrice di misura (misuriamo il tempo)
       self.H = [[0, 1]]
       
       # Covarianze
       self.P = [[1, 0], [0, 1]]
       self.Q = [[0.01, 0], [0, 0.01]]  # Rumore processo
       self.R = [[measurement_noise]]    # Rumore misura tempo

   def mat_mult(self, A, B):
       """Moltiplicazione matrici."""
       rows_A = len(A)
       cols_A = len(A[0])
       cols_B = len(B[0]) if isinstance(B[0], list) else 1
       
       result = [[0] * cols_B for _ in range(rows_A)]
       
       for i in range(rows_A):
           for j in range(cols_B):
               for k in range(cols_A):
                   if isinstance(B[k], list):
                       result[i][j] += A[i][k] * B[k][j]
                   else:
                       result[i][j] += A[i][k] * B[k]
       return result

   def mat_add(self, A, B):
       """Somma matrici."""
       return [[A[i][j] + B[i][j] 
               for j in range(len(A[0]))] 
               for i in range(len(A))]

   def mat_transp(self, A):
       """Trasposta matrice."""
       return [[A[j][i] for j in range(len(A))] 
               for i in range(len(A[0]))]

   def predict(self):
       # x = F * x
       self.x = [sum(self.F[i][j] * self.x[j] 
                for j in range(2)) 
                for i in range(2)]
       
       # P = F * P * F.T + Q
       FP = self.mat_mult(self.F, self.P)
       FT = self.mat_transp(self.F)
       self.P = self.mat_add(
           self.mat_mult(FP, FT), 
           self.Q
       )

   def update(self, t_volo):
       # Innovazione y = z - H*x
       y = t_volo - sum(self.H[0][j] * self.x[j] 
                       for j in range(2))
       
       # S = H*P*H.T + R
       HP = self.mat_mult(self.H, self.P)
       HT = self.mat_transp(self.H)
       S = self.mat_add(
           self.mat_mult(HP, HT), 
           self.R
       )[0][0]
       
       # K = P*H.T/S
       PHT = self.mat_mult(self.P, HT)
       K = [[k[0]/S] for k in PHT]
       
       # x = x + K*y
       self.x = [self.x[i] + K[i][0] * y 
                for i in range(2)]
       
       # P = P - K*H*P
       KH = self.mat_mult(K, self.H)
       self.P = self.mat_add(
           self.P,
           [[-KH[i][j] * self.P[j][k] 
             for k in range(2)]
             for i, j in [(i,j) 
             for i in range(2) 
             for j in range(2)]]
       )
       
       # Calcolo altezza
       return (self.g * self.x[1]**2) / 8

def calcola_salto_micro(forze, freq_camp, peso_statico):
   dt = 1/freq_camp
   kf = KalmanJumpHeightMicro(dt)
   
   # Trova tempo di volo
   for i, f in enumerate(forze):
       if f < peso_statico * 0.2:
           t_stacco = i
           break
   
   for i, f in enumerate(forze[t_stacco:]):
       if f > peso_statico * 1.5:
           t_atterr = i + t_stacco
           break
   
   t_volo = (t_atterr - t_stacco) / freq_camp
   
   # Stima con Kalman
   kf.predict()
   altezza = kf.update(t_volo)
   
   return altezza, t_volo
```

>[Torna all'indice](readme.md#fasi-progetto)
