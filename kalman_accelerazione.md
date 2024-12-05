Per il calcolo dell'altezza basato sulle misure di accelerazione, utilizzando un modello deterministico, possiamo descrivere le matrici 
𝐹
F (dinamica del sistema) e 
𝐻
H (osservazione) necessarie per implementare un filtro di Kalman.

1. Modello del Sistema
Lo stato del sistema può essere descritto come un vettore:

𝑥
=
[
ℎ
𝑣
𝑎
]
x= 
​
  
h
v
a
​
  
​
 
Dove:

ℎ
h: altezza (posizione verticale).
𝑣
v: velocità verticale.
𝑎
a: accelerazione verticale relativa (calcolata dalla pedana).
Il sistema evolve secondo le equazioni di moto:

ℎ
(
𝑡
+
Δ
𝑡
)
=
ℎ
(
𝑡
)
+
𝑣
(
𝑡
)
⋅
Δ
𝑡
+
0.5
⋅
𝑎
(
𝑡
)
⋅
(
Δ
𝑡
)
2
𝑣
(
𝑡
+
Δ
𝑡
)
=
𝑣
(
𝑡
)
+
𝑎
(
𝑡
)
⋅
Δ
𝑡
𝑎
(
𝑡
+
Δ
𝑡
)
=
𝑎
(
𝑡
)
(assumendo accelerazione costante in ogni piccolo intervallo)
h(t+Δt)
v(t+Δt)
a(t+Δt)
​
  
=h(t)+v(t)⋅Δt+0.5⋅a(t)⋅(Δt) 
2
 
=v(t)+a(t)⋅Δt
=a(t)(assumendo accelerazione costante in ogni piccolo intervallo)
​
 
2. Matrice dello Stato 
𝐹
F
La matrice 
𝐹
F (transizione di stato) rappresenta la relazione tra lo stato attuale e quello futuro. Per un sistema discreto con passo temporale 
Δ
𝑡
Δt, la matrice sarà:

𝐹
=
[
1
Δ
𝑡
0.5
⋅
(
Δ
𝑡
)
2
0
1
Δ
𝑡
0
0
1
]
F= 
​
  
1
0
0
​
  
Δt
1
0
​
  
0.5⋅(Δt) 
2
 
Δt
1
​
  
​
 
Qui:

La prima riga rappresenta l'aggiornamento dell'altezza usando la velocità e l'accelerazione.
La seconda riga aggiorna la velocità usando l'accelerazione.
La terza riga assume che l'accelerazione rimanga costante nel breve intervallo.
3. Matrice di Osservazione 
𝐻
H
La matrice 
𝐻
H mappa lo stato reale sulle osservazioni disponibili. La pedana misura la forza totale, da cui si calcola l'accelerazione relativa:

𝑎
relativa
(
𝑡
)
=
𝑔
⋅
𝐹
pedana
(
𝑡
)
−
𝐹
statico
𝐹
statico
a 
relativa
​
 (t)=g⋅ 
F 
statico
​
 
F 
pedana
​
 (t)−F 
statico
​
 
​
 
Quindi la misura diretta è solo l'accelerazione. La matrice 
𝐻
H sarà:

𝐻
=
[
0
0
1
]
H=[ 
0
​
  
0
​
  
1
​
 ]
Questo significa che il sistema osserva solo l'accelerazione 
𝑎
a, senza misurare direttamente altezza o velocità.

4. Rumore del Sistema e delle Misure
Rumore di processo (
𝑄
Q): rappresenta l'incertezza nel modello deterministico, come errori nella stima di 
Δ
𝑡
Δt o accelerazioni variabili.
Rumore di misura (
𝑅
R): rappresenta l'incertezza nelle misure dell'accelerazione.
5. Integrazione con il Modello Deterministico
Predizione dello stato: Usando la matrice 
𝐹
F, prevediamo l'evoluzione di altezza, velocità e accelerazione.

𝑥
pred
=
𝐹
⋅
𝑥
prev
x 
pred
​
 =F⋅x 
prev
​
 
Aggiornamento con le osservazioni: Confrontiamo la misura dell'accelerazione calcolata dalla pedana con quella prevista dal modello:

𝑦
=
𝑧
−
𝐻
⋅
𝑥
pred
y=z−H⋅x 
pred
​
 
Dove 
𝑧
z è l'accelerazione misurata dalla pedana.

Calcolo della velocità e altezza: Una volta che 
𝑥
x è aggiornato, possiamo integrare la velocità e l'altezza con le relazioni cinematiche descritte sopra.

Implementazione Pratica
Stato iniziale: Si assume 
ℎ
(
0
)
=
0
h(0)=0 (altezza iniziale), 
𝑣
(
0
)
=
0
v(0)=0 (a riposo) e 
𝑎
(
0
)
a(0) stimato dalla pedana.
Passo temporale 
Δ
𝑡
Δt: Determinato dalla frequenza di campionamento della pedana.
Integrazione numerica: L'altezza massima si trova quando la velocità verticale 
𝑣
v diventa zero.
