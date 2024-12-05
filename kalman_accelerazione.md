# **Filtro di Kalman per il Calcolo dell'Altezza**

Questo progetto implementa un filtro di Kalman per stimare l'altezza, la velocità e l'accelerazione verticale partendo da misure di forza o accelerazione. Utilizza un modello deterministico basato sulle equazioni del moto.

---

## **Modello del Sistema**

### Stato del Sistema
Lo stato del sistema è rappresentato come un vettore:
\[
x = 
\begin{bmatrix}
h \\
v \\
a
\end{bmatrix}
\]

Dove:
- \( h \): altezza (posizione verticale).
- \( v \): velocità verticale.
- \( a \): accelerazione verticale relativa.

### Dinamica del Sistema
L'evoluzione del sistema è descritta dalle equazioni di moto:
\[
h(t + \Delta t) = h(t) + v(t) \cdot \Delta t + 0.5 \cdot a(t) \cdot (\Delta t)^2
\]
\[
v(t + \Delta t) = v(t) + a(t) \cdot \Delta t
\]
\[
a(t + \Delta t) = a(t) \quad (\text{accelerazione costante su piccoli intervalli})
\]

---

## **Matrici del Modello**

### Matrice di Transizione dello Stato \( F \)
La matrice \( F \) descrive la relazione tra lo stato attuale e quello futuro, considerando un passo temporale discreto \(\Delta t\):
\[
F = 
\begin{bmatrix}
1 & \Delta t & 0.5 \cdot (\Delta t)^2 \\
0 & 1 & \Delta t \\
0 & 0 & 1
\end{bmatrix}
\]
- La prima riga aggiorna l'altezza usando velocità e accelerazione.
- La seconda riga aggiorna la velocità usando l'accelerazione.
- La terza riga assume accelerazione costante in \(\Delta t\).

### Matrice di Osservazione \( H \)
La matrice \( H \) mappa lo stato reale sulle misure disponibili. Poiché si misura solo l'accelerazione relativa, \( H \) è:
\[
H = 
\begin{bmatrix}
0 & 0 & 1
\end{bmatrix}
\]

### Rumore
- **Rumore di processo \( Q \)**: rappresenta incertezze nel modello (es. accelerazioni variabili).
- **Rumore di misura \( R \)**: rappresenta incertezze nelle misure di accelerazione.

---

## **Filtro di Kalman**

### Predizione dello Stato
La fase di predizione calcola lo stato futuro usando il modello:
\[
x_{\text{pred}} = F \cdot x_{\text{prev}}
\]

La covarianza dello stato viene aggiornata:
\[
P_{\text{pred}} = F \cdot P_{\text{prev}} \cdot F^T + Q
\]

### Aggiornamento con le Osservazioni
Si confronta la misura osservata (\( z \)) con quella prevista:
\[
y = z - H \cdot x_{\text{pred}}
\]
\[
S = H \cdot P_{\text{pred}} \cdot H^T + R
\]
\[
K = P_{\text{pred}} \cdot H^T \cdot S^{-1}
\]

Lo stato e la covarianza vengono aggiornati:
\[
x = x_{\text{pred}} + K \cdot y
\]
\[
P = (I - K \cdot H) \cdot P_{\text{pred}}
\]

---

## **Implementazione Pratica**

1. **Stato Iniziale**:
   \[
   h(0) = 0, \quad v(0) = 0, \quad a(0) = \text{stimato dalla pedana.}
   \]
2. **Passo Temporale**: Determinato dalla frequenza di campionamento.
3. **Altezza Massima**: Si trova quando \( v = 0 \).

---

## **Esempio di Calcolo**

### Esempio di Matrici
Con \(\Delta t = 0.1\) secondi:
\[
F = 
\begin{bmatrix}
1 & 0.1 & 0.005 \\
0 & 1 & 0.1 \\
0 & 0 & 1
\end{bmatrix}
\]
\[
H = 
\begin{bmatrix}
0 & 0 & 1
\end{bmatrix}
\]

### Simulazione delle Misure
Supponiamo di avere misure di accelerazione:
\[
z = [0.2, 0.25, 0.3, 0.35, 0.4]
\]

Il filtro di Kalman stimerà altezza e velocità iterativamente.

---

## **Codice di Implementazione**

Per l'implementazione in **Python** e **MicroPython**, consulta il file [kalman_filter.py](./kalman_filter.py).

---

## **Autore**
Progetto sviluppato per applicazioni di analisi biomeccanica e controllo dei sistemi dinamici.
