Calcolo dell'Altezza con Filtro di Kalman
1. Modello del Sistema
Lo stato del sistema può essere descritto come un vettore:
Copyx = [h, v, a]^T
Dove:

h: altezza (posizione verticale)
v: velocità verticale
a: accelerazione verticale relativa (calcolata dalla pedana)

Il sistema evolve secondo le equazioni di moto:
Copyh(t+Δt) = h(t) + v(t) · Δt + 0.5 · a(t) · (Δt)²
v(t+Δt) = v(t) + a(t) · Δt
a(t+Δt) = a(t)  (assumendo accelerazione costante in ogni piccolo intervallo)
2. Matrice dello Stato F
La matrice F (transizione di stato) per un sistema discreto con passo temporale Δt:
CopyF = [
    1   Δt   0.5 · (Δt)²
    0    1        Δt
    0    0         1
]
Dettagli della Matrice F:

Prima riga: aggiornamento dell'altezza usando velocità e accelerazione
Seconda riga: aggiorna la velocità usando l'accelerazione
Terza riga: assume accelerazione costante nel breve intervallo

3. Matrice di Osservazione H
Calcolo dell'accelerazione relativa:
Copya_relativa(t) = g · (F_pedana(t) - F_statico) / F_statico
Matrice H:
CopyH = [0, 0, 1]
Significa che il sistema osserva solo l'accelerazione a.
4. Rumore del Sistema

Rumore di processo (Q): incertezza nel modello deterministico
Rumore di misura (R): incertezza nelle misure dell'accelerazione

5. Integrazione del Modello Deterministic
Predizione dello stato:
Copyx_pred = F · x_prev
Aggiornamento con le osservazioni:
Copyy = z - H · x_pred
Dove z è l'accelerazione misurata dalla pedana.
Implementazione Pratica

Stato iniziale:

h(0) = 0
v(0) = 0
a(0) stimato dalla pedana


Passo temporale Δt:
Determinato dalla frequenza di campionamento della pedana
Calcolo altezza massima:
Quando la velocità verticale v diventa zero
