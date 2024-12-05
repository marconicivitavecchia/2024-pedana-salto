Ecco la versione con formattazione matematica per Markdown:
Calcolo dell'Altezza con Filtro di Kalman
1. Modello del Sistema
Lo stato del sistema può essere descritto come un vettore:
$$x = \begin{bmatrix} h \ v \ a \end{bmatrix}$$
Dove:

$h$: altezza (posizione verticale)
$v$: velocità verticale
$a$: accelerazione verticale relativa (calcolata dalla pedana)

Il sistema evolve secondo le equazioni di moto:
$$\begin{aligned}
h(t+\Delta t) &= h(t) + v(t) \cdot \Delta t + 0.5 \cdot a(t) \cdot (\Delta t)^2 \
v(t+\Delta t) &= v(t) + a(t) \cdot \Delta t \
a(t+\Delta t) &= a(t) \quad \text{(assumendo accelerazione costante in ogni piccolo intervallo)}
\end{aligned}$$
2. Matrice dello Stato F
La matrice F (transizione di stato) per un sistema discreto con passo temporale $\Delta t$:
$$F = \begin{bmatrix}
1 & \Delta t & 0.5 \cdot (\Delta t)^2 \
0 & 1 & \Delta t \
0 & 0 & 1
\end{bmatrix}$$
Dettagli della Matrice F:

Prima riga: aggiornamento dell'altezza usando velocità e accelerazione
Seconda riga: aggiorna la velocità usando l'accelerazione
Terza riga: assume accelerazione costante nel breve intervallo

3. Matrice di Osservazione H
Calcolo dell'accelerazione relativa:
$$a_{\text{relativa}}(t) = g \cdot \frac{F_{\text{pedana}}(t) - F_{\text{statico}}}{F_{\text{statico}}}$$
Matrice H:
$$H = \begin{bmatrix} 0 & 0 & 1 \end{bmatrix}$$
Significa che il sistema osserva solo l'accelerazione $a$.
4. Rumore del Sistema

Rumore di processo (Q): incertezza nel modello deterministico
Rumore di misura (R): incertezza nelle misure dell'accelerazione

5. Integrazione del Modello Deterministico
Predizione dello stato:
$$x_{\text{pred}} = F \cdot x_{\text{prev}}$$
Aggiornamento con le osservazioni:
$$y = z - H \cdot x_{\text{pred}}$$
Dove $z$ è l'accelerazione misurata dalla pedana.
Implementazione Pratica

Stato iniziale:

$h(0) = 0$
$v(0) = 0$
$a(0)$ stimato dalla pedana


Passo temporale $\Delta t$:
Determinato dalla frequenza di campionamento della pedana
Calcolo altezza massima:
Quando la velocità verticale $v$ diventa zero
