>[Torna all'indice](readme.md#fasi-progetto)

# Calcolo dell'Altezza con Filtro di Kalman

## 1. Modello del Sistema

Lo stato del sistema può essere descritto come un vettore:

$$x = \begin{bmatrix} h \\ v \\ a \end{bmatrix}$$

Dove:
- $h$: altezza (posizione verticale)
- $v$: velocità verticale 
- $a$: accelerazione verticale relativa (calcolata dalla pedana)

Il sistema evolve secondo le equazioni di moto:

$$h(t+\Delta t) = h(t) + v(t) \cdot \Delta t + 0.5 \cdot a(t) \cdot (\Delta t)^2$$

$$v(t+\Delta t) = v(t) + a(t) \cdot \Delta t$$

$$a(t+\Delta t) = a(t) \quad \text{(assumendo accelerazione costante in ogni piccolo intervallo)}$$

## 2. Matrice dello Stato F

La matrice F (transizione di stato) per un sistema discreto con passo temporale $\Delta t$:

$$F = \begin{bmatrix} 
1 & \Delta t & 0.5 \cdot (\Delta t)^2 \\
0 & 1 & \Delta t \\
0 & 0 & 1 
\end{bmatrix}$$

### Dettagli della Matrice F:
- Prima riga: aggiornamento dell'altezza usando velocità e accelerazione
- Seconda riga: aggiorna la velocità usando l'accelerazione
- Terza riga: assume accelerazione costante nel breve intervallo

## 3. Matrice di Osservazione H

Calcolo dell'accelerazione relativa:

$$a_{\text{relativa}}(t) = g \cdot \frac{F_{\text{pedana}}(t) - F_{\text{statico}}}{F_{\text{statico}}}$$

Matrice H:

$$H = \begin{bmatrix} 0 & 0 & 1 \end{bmatrix}$$

Significa che il sistema osserva solo l'accelerazione $a$.

## 4. Rumore del Sistema

- **Rumore di processo (Q)**: incertezza nel modello deterministico
- **Rumore di misura (R)**: incertezza nelle misure dell'accelerazione

## 5. Integrazione del Modello Deterministico

### Predizione dello stato:
$$x_{\text{pred}} = F \cdot x_{\text{prev}}$$

### Aggiornamento con le osservazioni:
$$y = z - H \cdot x_{\text{pred}}$$
Dove $z$ è l'accelerazione misurata dalla pedana.

## Implementazione Pratica

- **Stato iniziale**:
  - $h(0) = 0$
  - $v(0) = 0$
  - $a(0)$ stimato dalla pedana

- **Passo temporale $\Delta t$**: 
  Determinato dalla frequenza di campionamento della pedana

- **Calcolo altezza massima**: 
  Quando la velocità verticale $v$ diventa zero


Implementazione in Python
  
```python

pythonCopyimport numpy as np

class KalmanFilter:
    def __init__(self, dt, process_noise, measurement_noise):
        self.dt = dt  # Intervallo di tempo
        
        # Stato iniziale [altezza, velocità, accelerazione]
        self.x = np.array([0, 0, 0], dtype=float)
        
        # Matrice di transizione dello stato F
        self.F = np.array([
            [1, dt, 0.5 * dt ** 2],
            [0, 1, dt],
            [0, 0, 1]
        ])
        
        # Matrice di osservazione H
        self.H = np.array([[0, 0, 1]])
        
        # Matrice di covarianza iniziale P
        self.P = np.eye(3)
        
        # Rumore di processo Q
        self.Q = np.eye(3) * process_noise
        
        # Rumore di misura R
        self.R = np.array([[measurement_noise]])

    def predict(self):
        # Predizione dello stato
        self.x = np.dot(self.F, self.x)
        # Aggiornamento della covarianza
        self.P = np.dot(np.dot(self.F, self.P), self.F.T) + self.Q

    def update(self, z):
        # Calcolo dell'innovazione
        y = z - np.dot(self.H, self.x)
        # Calcolo del guadagno di Kalman
        S = np.dot(self.H, np.dot(self.P, self.H.T)) + self.R
        K = np.dot(np.dot(self.P, self.H.T), np.linalg.inv(S))
        # Aggiornamento dello stato
        self.x = self.x + np.dot(K, y)
        # Aggiornamento della covarianza
        I = np.eye(len(self.x))
        self.P = np.dot(I - np.dot(K, self.H), self.P)

    def get_state(self):
        return self.x
```

Implementazione in MicroPython

```python

class KalmanFilter:
    def __init__(self, dt, process_noise, measurement_noise):
        self.dt = dt
        self.x = [0, 0, 0]  # h, v, a
        
        self.F = [
            [1, dt, 0.5 * dt ** 2],
            [0, 1, dt],
            [0, 0, 1]
        ]
        
        self.H = [[0, 0, 1]]
        self.P = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]
        self.Q = [[process_noise if i == j else 0 for j in range(3)] 
                 for i in range(3)]
        self.R = [[measurement_noise]]

    def mat_mult(self, A, B):
        return [[sum(A[i][k] * B[k][j] for k in range(len(B))) 
                for j in range(len(B[0]))] for i in range(len(A))]

    def mat_add(self, A, B):
        return [[A[i][j] + B[i][j] for j in range(len(A[0]))] 
                for i in range(len(A))]

    def predict(self):
        self.x = [
            sum(self.F[i][j] * self.x[j] for j in range(3)) 
            for i in range(3)
        ]
        FT = [[self.F[j][i] for j in range(3)] for i in range(3)]
        FP = self.mat_mult(self.F, self.P)
        self.P = self.mat_add(self.mat_mult(FP, FT), self.Q)

    def update(self, z):
        hx = sum(self.H[0][j] * self.x[j] for j in range(3))
        y = z - hx
        HP = self.mat_mult(self.H, self.P)
        HT = [[self.H[0][i]] for i in range(3)]
        S = self.mat_add(self.mat_mult(HP, HT), self.R)
        K = self.mat_mult(self.P, HT)
        K = [[k / S[0][0] for k in row] for row in K]
        self.x = [self.x[i] + K[i][0] * y for i in range(3)]
        I = [[1 if i == j else 0 for j in range(3)] for i in range(3)]
        KH = self.mat_mult(K, self.H)
        self.P = self.mat_mult(self.mat_add(I, [[-kh for kh in row] 
                                               for row in KH]), self.P)

    def get_state(self):
        return self.x
```

Utilizzo del Filtro

# Parametri di inizializzazione
dt = 0.01  # 10ms intervallo di campionamento
process_noise = 0.01
measurement_noise = 0.1

# Creazione filtro
kf = KalmanFilter(dt, process_noise, measurement_noise)

# Esempio di utilizzo
```python
measurements = [0.2, 0.25, 0.3, 0.35, 0.4]  # Accelerazioni misurate
for z in measurements:
    kf.predict()
    kf.update(z)
    state = kf.get_state()
    print(f"Altezza: {state[0]:.3f}m, Velocità: {state[1]:.3f}m/s")
```

Note Implementative

Differenze chiave tra le versioni:

Python: usa numpy per operazioni matriciali efficienti
MicroPython: implementa manualmente le operazioni matriciali


Parametri critici:

dt: intervallo di campionamento
process_noise: incertezza nel modello
measurement_noise: incertezza nelle misure

## Da Claude

Filtro di Kalman per Calcolo Altezza da Accelerazione della Pedana di Forza

Teoria Fisica
Base del Calcolo
L'accelerazione netta dell'atleta viene calcolata dalla misura di forza:
$$a = g(\frac{P_s}{P_0} - 1)$$
dove: 

a: accelerazione netta
g: accelerazione di gravità (9.81 m/s²)
Ps: forza misurata dalla pedana
P0: peso statico dell'atleta

Modello di Stato
Definiamo il vettore di stato:
$$mathCopyx = \begin{bmatrix} h \\ v \\ a \end{bmatrix}$$
dove:

h: altezza
v: velocità
a: accelerazione

Equazioni del Moto
$$y\begin{align*}$$
$$h_{k+1} &= h_k + v_k\Delta t + \frac{1}{2}a_k\Delta t^2 \\
v_{k+1} &= v_k + a_k\Delta t \\
a_{k+1} &= g(\frac{P_{s,k+1}}{P_0} - 1)
\end{align*}$$

1. Modello Fisico
Accelerazione dalla Pedana

$$a = g(\frac{P_s}{P_0} - 1)$$

Dove:

a: accelerazione netta [m/s²]
g: accelerazione di gravità (9.81 m/s²)
Ps: forza istantanea [N]
P0: peso statico [N]

Variabili di Stato

$$yx = \begin{bmatrix} h \\ v \\ a \end{bmatrix}$$

Equazioni del Moto

$$\begin{align*}
h_{k+1} &= h_k + v_k\Delta t + \frac{1}{2}a_k\Delta t^2 \\
v_{k+1} &= v_k + a_k\Delta t \\
a_{k+1} &= g(\frac{P_{s,k+1}}{P_0} - 1)
\end{align*}$$

2. Filtro di Kalman
Matrice di Transizione
$$yF = \begin{bmatrix} 
1 & \Delta t & \frac{1}{2}\Delta t^2 \\
0 & 1 & \Delta t \\
0 & 0 & 1
\end{bmatrix}$$

Matrice di Osservazione
$$H = \begin{bmatrix} 0 & 0 & 1 \end{bmatrix}


Implementazione Python/MicroPython

Matrici Q e R del filtro
Esempi pratici di utilizzo
Analisi delle prestazioni

Sistema di Misura Altezza da Pedana di Forza

3. Matrici di Covarianza
Rumore di Processo Q
mathCopyQ = \begin{bmatrix} 
\sigma^2_h & 0 & 0 \\
0 & \sigma^2_v & 0 \\
0 & 0 & \sigma^2_a
\end{bmatrix}
Rumore di Misura R
mathCopyR = [\sigma^2_{force}]
4. Implementazione Python

```python
import numpy as np

class ForceKalmanFilter:
    def __init__(self, dt, peso_statico, process_noise=0.01, measurement_noise=0.1):
        self.dt = dt
        self.P0 = peso_statico
        self.g = 9.81
        
        # Stato [h, v, a]
        self.x = np.array([0., 0., 0.])
        
        # Matrice transizione
        self.F = np.array([
            [1, dt, 0.5*dt**2],
            [0, 1, dt],
            [0, 0, 1]
        ])
        
        # Matrice osservazione
        self.H = np.array([[0, 0, 1]])
        
        # Covarianze
        self.P = np.eye(3)
        self.Q = np.eye(3) * process_noise
        self.R = np.array([[measurement_noise]])

    def force_to_acceleration(self, force):
        """Converte forza in accelerazione"""
        return self.g * (force/self.P0 - 1)

    def predict(self):
        """Predizione stato"""
        self.x = self.F @ self.x
        self.P = self.F @ self.P @ self.F.T + self.Q

    def update(self, force):
        """Aggiornamento con misura"""
        z = self.force_to_acceleration(force)
        
        # Innovazione
        y = z - self.H @ self.x
        
        # Guadagno Kalman
        S = self.H @ self.P @ self.H.T + self.R
        K = self.P @ self.H.T @ np.linalg.inv(S)
        
        # Aggiornamento stato
        self.x = self.x + K @ [y]
        self.P = (np.eye(3) - K @ self.H) @ self.P

    def get_state(self):
        return {'height': self.x[0],
                'velocity': self.x[1],
                'acceleration': self.x[2]}
```

5. Implementazione MicroPython

```python

class ForceKalmanFilterMicro:
    def __init__(self, dt, peso_statico, process_noise=0.01, measurement_noise=0.1):
        self.dt = dt
        self.P0 = peso_statico
        self.g = 9.81
        
        # Stato [h, v, a]
        self.x = [0.0, 0.0, 0.0]
        
        # Matrice transizione
        self.F = [
            [1, dt, 0.5*dt**2],
            [0, 1, dt],
            [0, 0, 1]
        ]
        
        # Matrice osservazione
        self.H = [[0, 0, 1]]
        
        # Covarianze
        self.P = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]
        self.Q = [[process_noise, 0, 0], 
                 [0, process_noise, 0],
                 [0, 0, process_noise]]
        self.R = [[measurement_noise]]

    def mat_mult(self, A, B):
        """Moltiplicazione matrici"""
        if isinstance(B[0], list):
            return [[sum(a*b for a,b in zip(row, col)) 
                    for col in zip(*B)] for row in A]
        return [[sum(a*b for a,b in zip(row, B))] 
                for row in A]

    def mat_add(self, A, B):
        """Somma matrici"""
        return [[A[i][j] + B[i][j] 
                for j in range(len(A[0]))]
                for i in range(len(A))]

    def mat_transp(self, A):
        """Trasposta matrice"""
        return [[A[j][i] for j in range(len(A))]
                for i in range(len(A[0]))]

    def force_to_acceleration(self, force):
        """Converte forza in accelerazione"""
        return self.g * (force/self.P0 - 1)

    def predict(self):
        """Predizione stato"""
        self.x = [sum(self.F[i][j] * self.x[j] 
                 for j in range(3)) 
                 for i in range(3)]
        
        FP = self.mat_mult(self.F, self.P)
        FT = self.mat_transp(self.F)
        self.P = self.mat_add(
            self.mat_mult(FP, FT),
            self.Q
        )

    def update(self, force):
        """Aggiornamento con misura"""
        z = self.force_to_acceleration(force)
        
        # Innovazione
        y = z - sum(self.H[0][j] * self.x[j] 
                   for j in range(3))
        
        # Calcolo guadagno Kalman
        HP = self.mat_mult(self.H, self.P)
        HT = self.mat_transp(self.H)
        S = self.mat_add(
            self.mat_mult(HP, HT),
            self.R
        )[0][0]
        
        PHT = self.mat_mult(self.P, HT)
        K = [[k[0]/S] for k in PHT]
        
        # Aggiornamento stato
        self.x = [self.x[i] + K[i][0] * y 
                 for i in range(3)]
        
        # Aggiornamento covarianza
        KH = self.mat_mult(K, self.H)
        self.P = self.mat_add(
            self.P,
            [[-KH[i][j] * self.P[j][k] 
              for k in range(3)]
              for i, j in [(i,j) 
              for i in range(3) 
              for j in range(3)]]
        )

    def get_state(self):
        return {
            'height': self.x[0],
            'velocity': self.x[1],
            'acceleration': self.x[2]
        }
```

7. Esempio di Utilizzo

```python
def analizza_salto(forze, dt, peso_statico):
    # Inizializza filtro
    kf = ForceKalmanFilter(dt, peso_statico)
    
    # Array per salvare risultati
    heights = []
    velocities = []
    accelerations = []
    
    # Processa ogni misura
    for force in forze:
        kf.predict()
        kf.update(force)
        state = kf.get_state()
        
        heights.append(state['height'])
        velocities.append(state['velocity'])
        accelerations.append(state['acceleration'])
    
    # Trova altezza massima
    max_height = max(heights)
    
    return {
        'max_height': max_height,
        'heights': heights,
        'velocities': velocities,
        'accelerations': accelerations
    }
```

>[Torna all'indice](readme.md#fasi-progetto)
