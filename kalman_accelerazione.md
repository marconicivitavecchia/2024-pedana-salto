Modello con Filtro di Kalman per Altezza da Accelerazione
Modello del Sistema
Lo stato del sistema è descritto dal vettore:
mathCopyx = \begin{bmatrix} h \\ v \\ a \end{bmatrix}
Dove:

h: altezza (posizione verticale)
v: velocità verticale
a: accelerazione verticale relativa

Il sistema evolve secondo le equazioni di moto:
mathCopy\begin{align*}
h(t+\Delta t) &= h(t) + v(t)\Delta t + 0.5a(t)(\Delta t)^2 \\
v(t+\Delta t) &= v(t) + a(t)\Delta t \\
a(t+\Delta t) &= a(t) \text{ (accelerazione costante nell'intervallo)}
\end{align*}
Matrice dello Stato F
La matrice F (transizione di stato) per un sistema discreto con passo temporale Δt:
mathCopyF = \begin{bmatrix} 
1 & \Delta t & 0.5(\Delta t)^2 \\
0 & 1 & \Delta t \\
0 & 0 & 1
\end{bmatrix}
Dove:

Prima riga: aggiornamento dell'altezza usando velocità e accelerazione
Seconda riga: aggiornamento della velocità usando accelerazione
Terza riga: accelerazione costante nel breve intervallo

Matrice di Osservazione H
La pedana misura la forza totale, da cui si calcola l'accelerazione relativa:
mathCopya_{relativa}(t) = g\frac{F_{pedana}(t) - F_{statico}}{F_{statico}}
La matrice H sarà:
mathCopyH = \begin{bmatrix} 0 & 0 & 1 \end{bmatrix}
Rumore del Sistema e delle Misure

Q: rappresenta l'incertezza nel modello deterministico
R: rappresenta l'incertezza nelle misure dell'accelerazione

Integrazione con il Modello Deterministico
Predizione dello stato
mathCopyx_{pred} = F \cdot x_{prev}
Aggiornamento con le osservazioni
mathCopyy = z - H \cdot x_{pred}
Dove z è l'accelerazione misurata dalla pedana.
Implementazione Pratica
Condizioni iniziali:
mathCopy\begin{align*}
h(0) &= 0 \text{ (altezza iniziale)} \\
v(0) &= 0 \text{ (a riposo)} \\
a(0) &= \text{ stimato dalla pedana}
\end{align*}

Passo temporale Δt: determinato dalla frequenza di campionamento della pedana
Integrazione numerica: l'altezza massima si trova quando la velocità verticale v diventa zero
