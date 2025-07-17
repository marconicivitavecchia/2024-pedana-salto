>[Torna all'indice](readme.md#fasi-progetto)

# CALCOLO ALTEZZA DA MISURE DI ACCELERAZIONE

L'accelerazione netta può essere calcolata come:

$$a = g \left( \frac{P_s}{P_0} - 1 \right)$$

Dove:

$$P_s​$$ è la forza totale misurata durante il salto.

$$P_0$$ è la forza a riposo (peso statico dell'atleta).

## Dimostrazione della formula

Questa formula deriva dall'applicazione della seconda legge di Newton durante la fase di caricamento del salto.

Analizziamo la situazione:

**Durante il caricamento del salto:**
- L'atleta esercita una forza $P_s$ sulla pedana
- Per il terzo principio di Newton, la pedana esercita una forza di reazione $P_s$ verso l'alto sull'atleta
- Il peso dell'atleta (forza gravitazionale) è $P_0 = mg$ verso il basso

**Applicando la seconda legge di Newton:**
$$\sum F = ma$$

Le forze che agiscono sull'atleta sono:
- Forza di reazione della pedana: $+P_s$ (verso l'alto)
- Peso: $-P_0$ (verso il basso)

Quindi:
$$P_s - P_0 = ma$$

Risolvendo per l'accelerazione:
$$a = \frac{P_s - P_0}{m}$$

Ma sappiamo che $P_0 = mg$, quindi $m = \frac{P_0}{g}$

Sostituendo:
$$a = \frac{P_s - P_0}{\frac{P_0}{g}} = \frac{(P_s - P_0) \cdot g}{P_0}$$

$$a = g \cdot \frac{P_s - P_0}{P_0} = g \left(\frac{P_s}{P_0} - 1\right)$$

**Interpretazione fisica:**
- Se $P_s = P_0$: $a = 0$ (atleta fermo)
- Se $P_s > P_0$: $a > 0$ (accelerazione verso l'alto)
- Se $P_s < P_0$: $a < 0$ (accelerazione verso il basso)

Il termine $\left(\frac{P_s}{P_0} - 1\right)$ rappresenta il "fattore di caricamento" rispetto al peso a riposo.

La tara della pedana è già inclusa nel calcolo del peso statico $$P_0$$​, quindi non serve sottrarre una massa separata come $$m_t$$.

Se tara e peso dell'atleta vengono campionati nello stesso momento e con lo stesso sistema, l'errore relativo associato a queste misure si cancella nei calcoli che utilizzano differenze relative.

Vediamo perché questo accade.

## Errore Relativo nelle Misure della Pedana

### Errore Relativo: Tara e Peso Statico

1. Errore relativo costante ($\varepsilon$) nel sistema:
   
$$T_{misurata} = T \cdot (1 + \varepsilon)$$
$$F_{0,misurato} = F_0 \cdot (1 + \varepsilon)$$
$$F_{s,misurato} = F_s \cdot (1 + \varepsilon)$$

## Calcolo dell'Accelerazione

Formula base:
$$a = g \cdot \left( \frac{F_s - F_0}{F_0} \right)$$

Con valori misurati:
$$a_{misurata} = g \cdot \left( \frac{F_{s,misurato} - F_{0,misurato}}{F_{0,misurato}} \right)$$

Sostituendo le espressioni:
$$a_{misurata} = g \cdot \left( \frac{F_s \cdot (1 + \varepsilon) - F_0 \cdot (1 + \varepsilon)}{F_0 \cdot (1 + \varepsilon)} \right)$$

Semplificando:
$$a_{misurata} = g \cdot \left( \frac{(F_s - F_0) \cdot (1 + \varepsilon)}{F_0 \cdot (1 + \varepsilon)} \right)$$
$$a_{misurata} = g \cdot \left( \frac{F_s - F_0}{F_0} \right)$$

### Implicazioni

1. **Robustezza**: L'errore relativo costante si cancella nel calcolo
2. **Calibrazione**: Rimane importante per errori non relativi (drift, bias)
3. **Precisione**: Sistema valido con precisione relativa accettabile

## Accelerazione

Ma con una pedana in realtà misuro l'accelerazione della pedana o quella dell'atleta? Integrandola trovo la velocità iniziale che invece è uguale per entrambi, pedana e atleta?

La risposta è che una pedana misura la forza applicata su di essa, e quindi, indirettamente, l'accelerazione della pedana stessa. Tuttavia, con alcune considerazioni, possiamo ottenere l'accelerazione e la velocità dell'atleta, dato che il sistema pedana-atleta è interconnesso durante il salto.

Vediamo passo passo come funziona il ragionamento e perché la velocità iniziale è effettivamente condivisa tra atleta e pedana al momento del distacco.

## Relazione tra Accelerazione Pedana e Atleta

L'accelerazione della pedana ($a_{pedana}$) non coincide con quella dell'atleta ($a_{atleta}$), ma i movimenti sono correlati.

## Velocità al Distacco

Al momento del distacco:
- L'atleta smette di esercitare forza sulla pedana
- La pedana si ferma, l'atleta continua il moto ascendente

Fino al distacco, le velocità sono identiche per il vincolo di contatto:

$$v_{\text{iniziale, atleta}} = v_{\text{pedana al distacco}}$$

La velocità iniziale dell'atleta può quindi essere calcolata integrando la forza misurata dalla pedana nel tempo.


## Integrazione dell'Accelerazione Relativa

### 1. Accelerazione relativa della pedana

$$a_{\text{relativa}}(t) = \frac{F_{\text{pedana}}(t) - F_{\text{statico}}}{F_{\text{statico}}} \cdot g$$

Dove:
- $F_{\text{pedana}}(t)$: Forza misurata dalla pedana nel tempo
- $F_{\text{statico}}$: Forza statica (peso dell'atleta)
- $g$: Accelerazione di gravità

### 2. Integrazione dell'accelerazione relativa

Velocità relativa:
$$v_{\text{relativa}}(t) = \int_{t_0}^{t_{\text{distacco}}} a_{\text{relativa}}(t) \, dt$$

Sostituendo $a_{\text{relativa}}(t)$:
$$v_{\text{relativa}}(t) = g \int_{t_0}^{t_{\text{distacco}}} \frac{F_{\text{pedana}}(t) - F_{\text{statico}}}{F_{\text{statico}}} \, dt$$

### 3. Velocità iniziale dell'atleta

$$v_{\text{iniziale}} = v_{\text{relativa}}(t_{\text{distacco}})$$

Quindi:
$$v_{\text{iniziale}} = g \int_{t_0}^{t_{\text{distacco}}} \frac{F_{\text{pedana}}(t) - F_{\text{statico}}}{F_{\text{statico}}} \, dt$$

### Considerazioni

1. **Peso assoluto non necessario**:
   - Calcolo basato solo su valori relativi rispetto a $F_{\text{statico}}$

2. **Velocità condivisa**:
   - Atleta e pedana hanno stessa velocità durante il contatto

3. **Importanza del distacco**:
   - Calcolo valido fino al momento $t_{\text{distacco}}$
  

## Integrale Discreto per il Calcolo della Velocità

### Formula Continua
$$v_{\text{iniziale}} = g \int_{t_0}^{t_{\text{distacco}}} \frac{F_{\text{pedana}}(t) - F_{\text{statico}}}{F_{\text{statico}}} \, dt$$

### Versione Discreta
$$v_{\text{iniziale}} = g \sum_{n=0}^{N-1} \frac{F_{\text{pedana}}[n] - F_{\text{statico}}}{F_{\text{statico}}} \cdot \Delta t$$

Dove:
- $F_{\text{pedana}}[n]$: Forza campionata al tempo $t_n$
- $F_{\text{statico}}$: Forza statica pre-salto
- $\Delta t$: Intervallo di campionamento
- $N$: Numero totale di campioni da $t_0$ a $t_{\text{distacco}}$

Esempio in python:

```python
# Dati
F_statico = 700  # Peso statico dell'atleta in N
g = 9.81         # Accelerazione di gravità in m/s^2
delta_t = 0.01   # Intervallo di campionamento in secondi
F_pedana = [700, 750, 800, 850, 900, 1000, 1200, 1400]  # Forza misurata (esempio)
N = len(F_pedana)  # Numero di campioni

# Calcolo velocità iniziale
v_iniziale = 0
for n in range(N):
    relativa = (F_pedana[n] - F_statico) / F_statico
    v_iniziale += relativa * delta_t

# Moltiplicazione per g per ottenere velocità iniziale in m/s
v_iniziale *= g

# Risultato
print("Velocità iniziale:", v_iniziale, "m/s")
```

## Calcolo della Velocità di Caduta

### Formula nel Dominio Continuo

L'accelerazione relativa:
$$a_{\text{relativa}}(t) = \frac{F_{\text{pedana}}(t) - F_{\text{statico}}}{F_{\text{statico}}} \cdot g$$

Velocità di caduta:
$$v_{\text{caduta}} = g \int_{t_{\text{impatto}}}^{t_{\text{stabilizzato}}} \frac{F_{\text{pedana}}(t) - F_{\text{statico}}}{F_{\text{statico}}} \, dt$$

### Formula nel Dominio Discreto

$$v_{\text{caduta}} = g \cdot \Delta t \cdot \sum_{n=M}^{N-1} \frac{F_{\text{pedana}}[n] - F_{\text{statico}}}{F_{\text{statico}}}$$

Dove:
- $M$: Primo campione dopo il contatto
- $N$: Campione finale (accelerazione zero)
- $F_{\text{pedana}}[n]$: Forza misurata al campione $n$
- $\Delta t$: Intervallo di campionamento

Ecco un codice Python per calcolare la velocità di caduta utilizzando i dati della forza misurata dalla pedana durante l'atterraggio.
```python
# Parametri e dati iniziali
g = 9.81  # Accelerazione di gravità (m/s^2)
F_statico = 700  # Peso statico dell'atleta in Newton (esempio)
delta_t = 0.01  # Intervallo di campionamento (secondi)

# Forza misurata dalla pedana durante l'atterraggio (esempio)
F_pedana = [700, 900, 1200, 1500, 1200, 900, 700]

# Calcolo della velocità di caduta
def calcola_velocita_caduta(F_pedana, F_statico, delta_t, g):
    v_caduta = 0  # Velocità iniziale
    inizio_impulso = False
    
    for F in F_pedana:
        # Individua l'inizio del contatto (quando F > F_statico)
        if F > F_statico:
            inizio_impulso = True
        
        # Calcola solo dopo il contatto
        if inizio_impulso:
            relativa = (F - F_statico) / F_statico  # Accelerazione relativa
            v_caduta += g * relativa * delta_t  # Somma discreta
    
    return v_caduta

# Calcolo della velocità di caduta
v_caduta = calcola_velocita_caduta(F_pedana, F_statico, delta_t, g)

# Risultato
print(f"Velocità di caduta: {v_caduta:.2f} m/s")
```

## Calcolo dell'Altezza dal Salto

### Formula dell'Altezza
$$h = \frac{v_{\text{iniziale}}^2}{2g}$$

Dove:
- $v_{\text{iniziale}}$: Velocità verticale iniziale
- $g = 9.81 \, \text{m/s}^2$: Accelerazione di gravità

### Derivazione dalla Conservazione dell'Energia

1. Energia cinetica iniziale:
   $$E_{\text{cinetica}} = \frac{1}{2} m v_{\text{iniziale}}^2$$

2. Energia potenziale al punto massimo:
   $$E_{\text{potenziale}} = m g h$$

Uguagliando le energie:
$$\frac{1}{2} m v_{\text{iniziale}}^2 = m g h$$

Risolvendo per $h$:
$$h = \frac{v_{\text{iniziale}}^2}{2g}$$

Dalla formula dell'altezza:

$$h = \frac{v_{\text{iniziale}}^2}{2g}$$

puoi ricavare la velocità iniziale:

$$v_{\text{iniziale}} = \sqrt{2gh}$$

Ora, se hai la formula della velocità iniziale in funzione del tempo, puoi sostituirla. Le possibili connessioni dipendono dal contesto:

**1. Se il tempo è il tempo di volo totale:**
Per un moto parabolico, il tempo di volo totale è:
$$t = \frac{2v_{\text{iniziale}}}{g}$$

Quindi: $v_{\text{iniziale}} = \frac{gt}{2}$

Sostituendo nella formula dell'altezza:
$$h = \frac{1}{2g} \cdot \left(\frac{gt}{2}\right)^2 = \frac{g^2t^2}{8g} = \frac{gt^2}{8}$$

# Conclusione: Metodo di Calcolo Indipendente

## Calcolo della Velocità Iniziale
La pedana opera in modo autonomo attraverso:

1. **Misurazione peso statico**
   - Acquisizione diretta con atleta fermo

2. **Analisi differenze relative**
   - Rapporto tra forza dinamica e statica

3. **Integrazione accelerazione relativa**
   - Calcolo velocità iniziale

## Vantaggi
Il metodo è:
- Indipendente dal peso assoluto
- Autonomo nei calcoli
- Versatile nell'utilizzo

Non richiede parametri esterni come peso o massa dell'atleta.

## ✅ SIpossono usare direttamente i "counts" dell’ADC?

Ottima domanda — e la risposta è **molto interessante dal punto di vista pratico**:
✅ **No, non serve conoscere il valore assoluto di \$F\$ in Newton. Basta usare il valore grezzo dell’ADC (in divisioni o "counts")**, **purché** usi sempre la **stessa unità** per \$F\_s\$ e \$F\_0\$.

---

## ✅ Perché funziona anche con i "counts" dell’ADC?

La formula chiave per l’accelerazione relativa è:

$$
a = g \cdot \left( \frac{F_s - F_0}{F_0} \right)
$$

Ora, se sia \$F\_s\$ che \$F\_0\$ sono misurati dallo **stesso ADC**, con lo **stesso guadagno**, lo **stesso ponte di Wheatstone** o la **stessa cella di carico**, allora possiamo scrivere:

$$
F_s = K \cdot D_s \quad \text{e} \quad F_0 = K \cdot D_0
$$

dove:

* \$D\_s\$ è la lettura dell’ADC durante il salto (in counts)
* \$D\_0\$ è la lettura dell’ADC a riposo
* \$K\$ è il fattore di conversione tra counts e Newton (che **non serve conoscere**)

Quindi:

$$
a = g \cdot \left( \frac{K D_s - K D_0}{K D_0} \right)
= g \cdot \left( \frac{D_s - D_0}{D_0} \right)
$$

👉 **Risultato**: puoi calcolare l'accelerazione relativa usando direttamente i valori **grezzi dell’ADC**, senza convertire in Newton.

---

## ✅ Questo vale anche per l’integrazione e il calcolo dell’altezza?

Sì, **assolutamente**, se:

* Usi lo stesso \$D\_0\$ come riferimento per tutta la durata del salto
* Non cambia il guadagno o la scala del tuo sistema ADC durante la registrazione

Tutta l'integrazione dell'accelerazione relativa è basata su:

$$
a[n] = g \cdot \frac{D[n] - D_0}{D_0}
$$

quindi anche **velocità e altezza finale** saranno corretti in metri al secondo e in metri.

---

## 📌 Attenzione a questi dettagli pratici

1. **\$D\_0\$ deve essere stabile**
   Fai una media su più campioni a riposo per ridurre il rumore.

2. **ADC senza offset o drift**
   Se l’ADC introduce un offset termico o non-lineare, potresti avere errori sistematici. Ma finché l’errore è **costante**, si annulla nel rapporto.

3. **Campionamento sufficientemente veloce**
   Se integri numericamente, serve una frequenza di campionamento **almeno 200 Hz**, meglio 500 Hz o più, per seguire bene la fase impulsiva del salto.

---

## ✅ Conclusione

Puoi **tranquillamente usare i valori grezzi dell’ADC** per tutto il tuo calcolo (accelerazione, velocità, altezza) — non è necessaria la conversione in Newton o chilogrammi **se usi sempre lo stesso riferimento statico** \$D\_0\$.

Se vuoi, posso aiutarti a scrivere una versione del tuo script che lavora direttamente con i counts ADC, senza calibrazione.


## Sitografia:
- http://lia.deis.unibo.it/Courses/InfoChim0910/lucidi/4-integrazioneDifferenziali(1x).pdf
- https://nsa.readthedocs.io/en/latest/03_integrali/docs/source/09_int_numerici.html
- https://www.tandfonline.com/doi/epdf/10.1080/02640414.2022.2059319

>[Torna all'indice](readme.md#fasi-progetto)
