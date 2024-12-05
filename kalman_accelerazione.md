

# **Filtro di Kalman per Altezza e Velocità**

Questo repository implementa un filtro di Kalman per stimare altezza, velocità e accelerazione di un atleta utilizzando misure di accelerazione fornite da una pedana di forza. Sono forniti esempi sia per Python standard che per MicroPython.

---

## **Descrizione del Problema**
### **Calcolo Altezza da Misure di Accelerazione**
L'accelerazione netta durante un salto può essere calcolata come:

\[
a = g \cdot \left( \frac{F_s}{F_0} - 1 \right)
\]

Dove:
- \(F_s\): Forza totale misurata durante il salto.
- \(F_0\): Forza a riposo (peso statico dell'atleta).
- \(g\): Accelerazione gravitazionale.

Il filtro di Kalman utilizza questo modello per stimare l'altezza e la velocità dell'atleta durante il movimento, considerando che:

1. **Velocità iniziale**: L'atleta e la pedana condividono la stessa velocità fino al distacco.
2. **Integrale discreto**: Calcolato per derivare la velocità relativa rispetto alla pedana.

### **Relazione tra Pedana e Atleta**
La velocità iniziale può essere calcolata integrando l'accelerazione relativa della pedana nel tempo:

\[
v_{\text{iniziale}} = g \cdot \int_{t_0}^{t_{\text{distacco}}} \frac{F_{\text{pedana}}(t) - F_{\text{statico}}}{F_{\text{statico}}} \, dt
\]

Versione discreta:

\[
v_{\text{iniziale}} = g \cdot \sum_{n=0}^{N-1} \frac{F_{\text{pedana}}[n] - F_{\text{statico}}}{F_{\text{statico}}} \cdot \Delta t
\]

---

## **Implementazioni**

### **1. Python**
La versione Python utilizza la libreria `numpy` per calcoli matriciali efficienti. È consigliata per ambienti con risorse hardware sufficienti.

#### **Codice Python**
```python
import numpy as np

class KalmanFilter:
    def __init__(self, dt, process_noise, measurement_noise):
        self.dt = dt  # Intervallo di tempo

        # Stato iniziale [altezza, velocità, accelerazione]
        self.x = np.array([0, 0, 0], dtype=float)  # h, v, a

        # Matrice di transizione dello stato F
        self.F = np.array([
            [1, dt, 0.5 * dt ** 2],
            [0, 1, dt],
            [0, 0, 1]
        ])

        # Matrice di osservazione H (misura solo accelerazione)
        self.H = np.array([[0, 0, 1]])

        # Matrice di covarianza iniziale P
        self.P = np.eye(3)

        # Rumore di processo Q
        self.Q = np.eye(3) * process_noise

        # Rumore di misura R
        self.R = np.array([[measurement_noise]])

    def predict(self):
        self.x = np.dot(self.F, self.x)
        self.P = np.dot(np.dot(self.F, self.P), self.F.T) + self.Q

    def update(self, z):
        y = z - np.dot(self.H, self.x)
        S = np.dot(self.H, np.dot(self.P, self.H.T)) + self.R
        K = np.dot(np.dot(self.P, self.H.T), np.linalg.inv(S))
        self.x = self.x + np.dot(K, y)
        I = np.eye(len(self.x))
        self.P = np.dot(I - np.dot(K, self.H), self.P)

    def get_state(self):
        return self.x

dt = 0.01
kf = KalmanFilter(dt, process_noise=0.01, measurement_noise=0.1)

measurements = [0.2, 0.25, 0.3, 0.35, 0.4]
for z in measurements:
    kf.predict()
    kf.update(z)
    print("Stato stimato:", kf.get_state())
