

# Calibrazione statica saltatore

E' una operazione che si fa prima di ogni salto.

Processo in 3 fasi:

**1. Lettura a Vuoto (Tara):**

   - I valori di uscita grezzi delle 4 celle vengono letti e sommati.
   - Questo valore rappresenta la tara totale del sistema.
   - questo valore è stato già salvato in fse di calibrazione statica della pedana

**2. Lettura con Peso di Riferimento:**

   - Un peso noto (ad esempio, 75Kg) viene posizionato sulla pedana.
   - I valori letti dalle 4 celle vengono sommati.

**3. Calcolo del Fattore di Scala:**

   - Il fattore di scala viene calcolato come:

      $$\text{Fattore di scala} = \frac{\text{Somma dei valori con peso} - \text{Somma dei valori di tara}}{\text{Peso noto}}$$
​
 
**4. Memorizzazione dei Parametri:**

   - I valori di tara per ciascuna cella e il fattore di scala complessivo vengono salvati in un file CSV insieme alle altre informazioni di calibrazione statica della pedana.

## Esempio di Utilizzo
1. Posizionare il saltatore quando richiesto.
3. I dati di calibrazione verranno salvati nel file /sd/calibrazione_sistema.csv.

# Calibrazione Statica di una Singola Cella di Carico

## Materiali necessari
- Un sistema per acquisire i dati da ciascuna cella (ad esempio, un ADC come l'ADS1256)
- Pesi noti di precisione (ad esempio, 1 kg, 2 kg, 5 kg)
- Un supporto stabile per applicare il peso in modo uniforme su ciascuna cella

## Procedura

### 1. Azzeramento della cella
- Collega la cella di carico al sistema di acquisizione e leggi il valore "a vuoto" (senza peso)
- Registra questo valore e sottrai questa lettura da tutte le misurazioni successive (offset)

### 2. Applicazione di pesi noti
- Applica un peso noto alla cella e registra l'output del sistema (ad esempio, la lettura in unità ADC o millivolt)
- Ripeti per una serie di pesi crescenti (ad esempio, 0 kg, 1 kg, 2 kg, 5 kg)

### 3. Calcolo del coefficiente di calibrazione
- Costruisci un grafico dell'output della cella (valore ADC o millivolt) rispetto al peso noto
- Calcola la pendenza della retta risultante (slope): `slope = Δoutput / Δpeso`
- Questo coefficiente sarà utilizzato per convertire l'output della cella in unità di peso o forza: `F = slope × output`

### 4. Verifica della linearità
- Verifica che la relazione sia lineare e, se necessario, correggi eventuali deviazioni con un'interpolazione o un'approssimazione polinomiale



# L'offset di una cella di carico

L'offset è la lettura che una cella di carico fornisce anche quando non c'è nessun peso applicato. Questo valore è dovuto a:

- Tensioni residue nel sistema
- Caratteristiche elettroniche del circuito
- Influenza ambientale

L'offset deve essere sottratto da ogni misura per ottenere il valore effettivo della forza applicata.

## Esempio Pratico calibrazione statica della pedana 

E'una operazione che si fa di tanto in tanto.

### Setup
- Hai una pedana con 4 celle di carico collegate a un ADC (es. ADS1256)
- Le letture vengono registrate in unità digitali (ad esempio, conteggi ADC)

### Step 1: Misura dell'offset
Nessun peso è applicato sulla pedana.
Registra le letture delle 4 celle:
```
F₁_offset = 1250
F₂_offset = 1265
F₃_offset = 1248
F₄_offset = 1255
```
Questi valori rappresentano l'offset per ciascuna cella.

### Step 2: Lettura con peso
Posizioni un peso noto W = 10 kg al centro della pedana.
Registra le letture delle celle:
```
F₁ = 2300
F₂ = 2400
F₃ = 2280
F₄ = 2350
```

### Step 3: Sottrazione dell'offset
Per ottenere le letture calibrate, sottrai l'offset misurato inizialmente da ciascun valore:
```
F₁_calibrato = F₁ - F₁_offset = 2300 - 1250 = 1050
F₂_calibrato = F₂ - F₂_offset = 2400 - 1265 = 1135
F₃_calibrato = F₃ - F₃_offset = 2280 - 1248 = 1032
F₄_calibrato = F₄ - F₄_offset = 2350 - 1255 = 1095
```

Le letture calibrate sono:
```
F₁_calibrato = 1050
F₂_calibrato = 1135
F₃_calibrato = 1032
F₄_calibrato = 1095
```

### Step 4: Calcolo della forza totale
La forza totale (o peso totale) può essere calcolata come:

```
F_totale = k₁⋅F₁_calibrato + k₂⋅F₂_calibrato + k₃⋅F₃_calibrato + k₄⋅F₄_calibrato
```

Se i coefficienti di calibrazione kᵢ sono uguali (k₁ = k₂ = k₃ = k₄ = k), il calcolo si semplifica:

```
F_totale = k⋅(F₁_calibrato + F₂_calibrato + F₃_calibrato + F₄_calibrato)
```

Supponendo k = 0.01 kg/conteggio:
```
F_totale = 0.01⋅(1050 + 1135 + 1032 + 1095) = 0.01⋅4312 = 43.12 kg
```

Il peso totale misurato è 43.12 kg.

## Conclusione
La sottrazione dell'offset consente di ottenere letture corrette e affidabili eliminando contributi estranei. Questo processo è essenziale per garantire che le misurazioni siano accurate.


# Determinazione dei Coefficienti di Calibrazione

## Ricapitoliamo i dati dell'esempio

### Offset misurati (senza peso)
```
F₁_offset = 1250
F₂_offset = 1265
F₃_offset = 1248
F₄_offset = 1255
```

### Letture con un peso noto W = 10 kg al centro
```
F₁ = 2300
F₂ = 2400
F₃ = 2280
F₄ = 2350
```

### Letture calibrate (dopo sottrazione dell'offset)
```
F₁_calibrato = 1050
F₂_calibrato = 1135
F₃_calibrato = 1032
F₄_calibrato = 1095
```

## Formula per calcolare kᵢ
Il coefficiente di calibrazione per ogni cella si calcola come:

```
kᵢ = W / Fᵢ_calibrato
```

Dove:
- W è il peso noto applicato (10 kg)
- Fᵢ_calibrato è la lettura calibrata della cella i

### Calcolo dei kᵢ
```
k₁ = 10/1050 ≈ 0.00952
k₂ = 10/1135 ≈ 0.00881
k₃ = 10/1032 ≈ 0.00969
k₄ = 10/1095 ≈ 0.00913
```

## Risultati
I coefficienti di calibrazione delle celle sono:
- k₁ ≈ 0.00952 kg/conteggio
- k₂ ≈ 0.00881 kg/conteggio
- k₃ ≈ 0.00969 kg/conteggio
- k₄ ≈ 0.00913 kg/conteggio

## Uso dei kᵢ
Con questi valori, puoi calcolare il peso totale applicato alla pedana in ogni condizione di misura:

```
W_totale = k₁⋅F₁_calibrato + k₂⋅F₂_calibrato + k₃⋅F₃_calibrato + k₄⋅F₄_calibrato
```

Ad esempio, se in una misura successiva le letture calibrate sono:
```
F₁_calibrato = 1200
F₂_calibrato = 1150
F₃_calibrato = 1180
F₄_calibrato = 1220
```

Il peso totale è:
```
W_totale = (0.00952⋅1200) + (0.00881⋅1150) + (0.00969⋅1180) + (0.00913⋅1220)
```

## Verifica della calibrazione
Dopo aver determinato i kᵢ, verifica che il peso totale calcolato con W_totale sia vicino al peso noto W usato durante la calibrazione. Se ci sono discrepanze, controlla:

1. Che il peso noto sia stato posizionato al centro della pedana
2. Che le celle siano correttamente livellate e funzionanti
3. Eventuali errori nell'acquisizione dei dati

L'accelerazione netta può essere calcolata come:

$$a = g \left( \frac{P_s}{P_0} - 1 \right)$$

Dove:

$$P_s​$$ è la forza totale misurata durante il salto.

$$P_0$$ è la forza a riposo (peso statico dell'atleta).
