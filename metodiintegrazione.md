

# Metodi FDI, BDI e FT+C per il Calcolo dell'Altezza del Salto Verticale

## Parte I: Fondamenti Teorici

### Introduzione

Il calcolo dell'altezza del salto verticale rappresenta una delle misurazioni più importanti nella valutazione della performance atletica e nella ricerca biomeccanica. Questo documento presenta i tre metodi principali utilizzati: Forward Double Integration (FDI), Backward Double Integration (BDI) e Flight Time + Constant (FT+C).

### Definizione dell'Altezza del Salto

L'altezza del salto è definita come la **distanza verticale tra il centro di massa (CoM) durante quiet standing e l'apice del salto**. Questa definizione include tutto il lavoro svolto per spingere il CoM nell'aria.

```
h_totale = h_heel_lift + h_aria
```

Dove:
- **h_heel_lift**: Spostamento del CoM dalla posizione di riposo al decollo
- **h_aria**: Spostamento del CoM durante la fase di volo

## 1. Forward Double Integration (FDI)

### Principi Teorici

Il metodo FDI si basa sulla **seconda legge di Newton** applicata al movimento del centro di massa:

```
F = m × a  →  a = F/m
```

### Formule Matematiche Fondamentali

#### Calcolo dell'accelerazione del CoM:
```
a(t) = (F_netta(t) / m) - g
```

#### Prima integrazione (accelerazione → velocità):
```
v(t) = ∫ a(t) dt = v₀ + ∫ a(t) dt
```

#### Seconda integrazione (velocità → spostamento):
```
s(t) = ∫ v(t) dt = s₀ + ∫ v(t) dt
```

#### Altezza totale del salto:
```
h_totale = s(t_decollo) + v²_decollo / (2g)
```

### Formulazione Integrale Continua

Per una comprensione teorica completa, le equazioni continue sono:

#### Sistema di Equazioni Differenziali
```
Equazione del moto:    F_netta(t) = m × a(t)
Prima integrazione:    v(t) = v₀ + ∫₀ᵗ a(τ) dτ
Seconda integrazione:  s(t) = s₀ + ∫₀ᵗ v(τ) dτ
```

#### Forma Integrale Esplicita
```
v(t) = v₀ + (1/m) ∫₀ᵗ [F(τ) - mg] dτ

s(t) = s₀ + v₀t + (1/m) ∫₀ᵗ ∫₀ᵘ [F(τ) - mg] dτ du
```

#### Condizioni al Contorno
```
Condizioni iniziali (quiet standing):
- t = 0: v(0) = 0, s(0) = 0
- F(0) = mg (equilibrio statico)

Condizioni al decollo:
- t = T: F(T) = 0 (perdita contatto)
- v(T) = v_decollo, s(T) = h_heel_lift
```

#### Principio di Conservazione dell'Energia
```
Lavoro totale = Energia cinetica + Energia potenziale

∫₀ᵀ F_netta(t) × v(t) dt = ½mv²_decollo + mg × h_heel_lift
```

### Implementazione Numerica Discreta

Nella pratica, i dati di forza sono campionati a frequenza finita, richiedendo l'uso di **integrazione numerica discreta** che approssima le formule continue.

#### Discretizzazione delle Equazioni Continue
Le equazioni integrali continue vengono approssimate con sommatorie discrete:

```
∫₀ᵗ a(τ) dτ  ≈  Σᵢ₌₁ⁿ a[i] × Δt    (approssimazione rettangoli)

∫₀ᵗ a(τ) dτ  ≈  Σᵢ₌₁ⁿ (a[i] + a[i-1])/2 × Δt    (regola trapezi)
```

#### Formule Discrete per FDI

**Step 1: Calcolo forza netta**
```
F_netta[i] = F_totale[i] - peso_corporeo
```

**Step 2: Calcolo accelerazione**
```
a[i] = F_netta[i] / m
```
dove `m = peso_corporeo / g`

**Step 3: Prima integrazione numerica (Metodo dei Trapezi)**
```
v[i] = v[i-1] + (a[i] + a[i-1]) × Δt / 2
```
con condizioni iniziali: `v[0] = 0`, `Δt = 1/frequenza_campionamento`

**Alternativa (Metodo di Eulero):**
```
v[i] = v[i-1] + a[i] × Δt
```

**Step 4: Seconda integrazione numerica**
```
s[i] = s[i-1] + (v[i] + v[i-1]) × Δt / 2    (Trapezi)
```
oppure:
```
s[i] = s[i-1] + v[i] × Δt                   (Eulero)
```
con condizioni iniziali: `s[0] = 0`

**Step 5: Identificazione del decollo**
```
i_decollo = primo indice dove F_netta[i] < -peso_corporeo
```

**Step 6: Calcolo componenti altezza**
```
h_heel_lift = s[i_decollo]
v_decollo = v[i_decollo]
h_aria = v²_decollo / (2g)
h_totale = h_heel_lift + h_aria
```

### Relazione tra Formulazione Continua e Discreta

#### Teorema Fondamentale
```
lim(Δt→0) Σᵢ₌₁ⁿ f[i] × Δt = ∫₀ᵀ f(t) dt
```

La formulazione discreta **converge** alla soluzione continua quando Δt → 0.

#### Errore di Approssimazione
Per il metodo dei trapezi:
```
Errore_globale ≈ -(T × Δt²/12) × max|f''(t)|
```

Questo giustifica teoricamente l'uso dell'integrazione numerica come approssimazione valida delle equazioni continue.

### Confronto Metodi di Integrazione Numerica

#### Metodo di Eulero (Forward Euler)
```
v[i] = v[i-1] + a[i] × Δt
s[i] = s[i-1] + v[i] × Δt
```
- **Vantaggi**: Semplicità computazionale
- **Svantaggi**: Accumulo errori maggiore, instabilità numerica
- **Ordine di accuratezza**: O(Δt)

#### Metodo dei Trapezi (Trapezoidal Rule)
```
v[i] = v[i-1] + (a[i] + a[i-1]) × Δt / 2
s[i] = s[i-1] + (v[i] + v[i-1]) × Δt / 2
```
- **Vantaggi**: Maggiore stabilità, errore ridotto
- **Svantaggi**: Leggero aumento computazionale
- **Ordine di accuratezza**: O(Δt²)

#### Metodo di Simpson (per dati ad alta frequenza)
```
v[i] = v[i-2] + Δt/3 × (a[i-2] + 4×a[i-1] + a[i])
```
- **Vantaggi**: Massima precisione
- **Svantaggi**: Richiede 3 punti, complessità maggiore
- **Ordine di accuratezza**: O(Δt⁴)

### Controllo degli Errori di Integrazione

#### Errore di Troncamento
L'errore locale per il metodo dei trapezi è:
```
ε_locale ≈ -(Δt³/12) × a''(t)
```

L'errore globale accumulato:
```
ε_globale ≈ -(T×Δt²/12) × max|a''(t)|
```
dove T è la durata totale dell'integrazione.

#### Criteri di Stabilità
Per garantire stabilità numerica:
```
Δt < 2/ω_max
```
dove ω_max è la frequenza massima significativa nel segnale (tipicamente 50-100 Hz per il movimento umano).

#### Raccomandazioni per Frequenza di Campionamento
```
f_min = 20 × f_movimento = 20 × 50 Hz = 1000 Hz
f_raccomandata = 2000 Hz (per margine di sicurezza)
```

### Condizioni di Applicabilità

- **Dati di forza completi** durante la fase di spinta
- **Quiet standing iniziale** per stabilire condizioni iniziali note
- **Durata limitata** della fase di integrazione (<2 secondi per limitare accumulo errori)

### Vantaggi
- **Massima precisione** (variabilità ±1mm)
- **Informazioni complete**: fornisce sia heel-lift che air height separatamente
- **Gold standard** per countermovement jump
- **Analisi dettagliata** delle componenti del movimento

### Limitazioni
- **Non applicabile** ai drop jump (manca quiet standing iniziale)
- **Problematico** per squat jump lunghi (accumulo errori di integrazione)
- **Sensibile al rumore** nei dati di forza
- **Richiede calibrazione precisa** del peso corporeo

## 2. Backward Double Integration (BDI)

### Principi Teorici

Il metodo BDI sfrutta il **principio di conservazione dell'energia**: tutta l'energia generata durante la spinta deve essere dissipata durante l'atterraggio.

### Il Problema delle Condizioni Iniziali

L'integrazione numerica richiede sempre condizioni iniziali note:
- Per FDI: v(0) = 0, s(0) = 0 (quiet standing iniziale)
- Per BDI raw: v(0) = ?, s(0) = ? (condizioni al momento dell'impatto **sconosciute**)

### Soluzione: Inversione Temporale

Il BDI risolve il problema delle condizioni iniziali attraverso l'**inversione temporale dei dati**:

1. **Estrazione** dei dati della fase di atterraggio
2. **Inversione** della sequenza temporale
3. **Applicazione** dell'algoritmo FDI sui dati invertiti
4. **Interpretazione** del risultato come altezza del salto

### Perché Funziona l'Inversione

- Il **quiet standing finale** fornisce condizioni iniziali note: v(0) = 0, s(0) = 0
- Le **leggi fisiche sono simmetriche** nel tempo
- L'**energia totale** è conservata tra spinta e atterraggio

### Formule Matematiche per BDI

#### Formulazione Teorica Continua

Il metodo BDI si basa sul **principio di conservazione dell'energia** e sulla **simmetria temporale** delle leggi fisiche.

**Principio fondamentale:**
```
∫₀ᵀ F_spinta(t) × v_spinta(t) dt = ∫₀ᵀ' F_atterraggio(t) × v_atterraggio(t) dt
```

Dove l'energia dissipata durante l'atterraggio è uguale all'energia generata durante la spinta.

**Inversione temporale:**
```
t' = T_atterraggio - t
F_invertita(t) = F_atterraggio(T_atterraggio - t)
```

**Sistema di equazioni per dati invertiti:**
```
a_inv(t) = [F_invertita(t) - mg] / m
v_inv(t) = ∫₀ᵗ a_inv(τ) dτ     (con v_inv(0) = 0)
s_inv(t) = ∫₀ᵗ v_inv(τ) dτ     (con s_inv(0) = 0)

h_salto = max{s_inv(t)}
```

### Implementazione Numerica Discreta per BDI

Le formule di integrazione numerica sono **identiche** al FDI, ma applicate ai dati temporalmente invertiti:

#### Preprocessamento dei Dati
```
dati_atterraggio = F[i_atterraggio : i_quiet_finale]
dati_invertiti = reverse(dati_atterraggio)
```

#### Integrazione sui Dati Invertiti
```
F_netta_inv[i] = dati_invertiti[i] - peso_corporeo
a_inv[i] = F_netta_inv[i] / m

v_inv[i] = v_inv[i-1] + a_inv[i] × Δt     (con v_inv[0] = 0)
s_inv[i] = s_inv[i-1] + v_inv[i] × Δt     (con s_inv[0] = 0)
```

#### Estrazione Risultato
```
h_totale = max(s_inv[i])
v_ricostruita = |v_inv[N-1]|  (velocità finale = velocità decollo ricostruita)
```

### Identificazione Automatica delle Fasi

#### Rilevamento Inizio Atterraggio
```
i_atterraggio = primo indice dove F[i] > α × peso_corporeo
```
con α = 2.0 ÷ 3.0 (moltiplicatore tipico)

#### Rilevamento Fine Atterraggio (Quiet Standing)
```
Per i = i_atterraggio + N_min fino a length(F) - N_finestra:
    F_media = mean(F[i : i + N_finestra])
    Se |F_media - peso_corporeo| < ε × peso_corporeo:
        i_quiet_finale = i
        break
```
con:
- N_min = 100 campioni (100ms a 1kHz)
- N_finestra = 50 campioni (finestra di stabilità)
- ε = 0.1 (tolleranza 10%)

### Vantaggi
- **Universalità**: funziona per tutti i tipi di salto (CMJ, SJ, DJ)
- **Precisione equivalente** al FDI (variabilità ±2-3mm)
- **Robustezza**: meno sensibile agli errori di drift (fase più breve)
- **Condizioni iniziali sempre note** (quiet standing finale)

### Limitazioni
- **Solo altezza totale**: non può separare heel-lift e air height
- **Forze di picco elevate**: problemi potenziali con pedane portatili (clipping)
- **Richiede atterraggio completo**: necessita ritorno al quiet standing
- **Nessuna informazione** sulle fasi di spinta

## 3. Flight Time + Constant (FT+C)

### Principi Teorici

Il metodo FT+C combina la **cinematica del moto uniformemente accelerato** con una **correzione antropometrica**:

```
h_totale = h_volo + C_antropometrica
```

### Formule Matematiche per FT+C

#### Formulazione Teorica Continua

Il metodo FT+C si basa sulla **cinematica del moto uniformemente accelerato** durante la fase di volo.

**Equazioni del moto durante il volo:**
```
Posizione: y(t) = y₀ + v₀t - ½gt²
Velocità:  v(t) = v₀ - gt
```

**Condizioni al contorno:**
```
Al decollo (t = 0):     y(0) = 0, v(0) = v_decollo
All'apice (t = t_apice): v(t_apice) = 0, y(t_apice) = h_aria
All'atterraggio (t = t_volo): y(t_volo) = 0, v(t_volo) = -v_decollo
```

**Derivazione analitica:**

Dal vincolo v(t_apice) = 0:
```
0 = v_decollo - g × t_apice
t_apice = v_decollo / g
```

Dalla simmetria del moto parabolico:
```
t_volo = 2 × t_apice = 2v_decollo / g
v_decollo = g × t_volo / 2
```

Sostituendo nell'equazione della posizione all'apice:
```
h_aria = v_decollo × t_apice - ½g × t²_apice
h_aria = (g × t_volo / 2) × (t_volo / 2) - ½g × (t_volo / 2)²
h_aria = g × t²_volo / 4 - g × t²_volo / 8 = g × t²_volo / 8
```

#### Calcolo Tempo di Volo
Il tempo di volo viene estratto dai dati di forza identificando decollo e atterraggio:

```
i_decollo = primo indice dove F[i] < β × peso_corporeo
i_atterraggio = primo indice dove F[i] > β × peso_corporeo (dopo i_decollo)
t_volo = (i_atterraggio - i_decollo) / f_campionamento
```
con β = 0.1 ÷ 0.2 (soglia per rilevamento contatto/non-contatto)

#### Correzione Antropometrica

La formula empirica derivata da misurazioni antropometriche (Wade et al., 2020):

```
C_heel_lift = 0.88 × L_piede + S_suola - H_caviglia
```

Dove:
- L_piede: distanza malleolo mediale - punta scarpa (m)
- S_suola: spessore suola sotto 1° falange (m)  
- H_caviglia: distanza malleolo mediale - suolo (m)

#### Formula Finale Completa
```
h_base = g × t²_volo / 8
C_antropometrica = (0.88 × L_piede) + S_suola - H_caviglia
h_totale = h_base + C_antropometrica
```

### Validazione Numerica delle Formule

#### Test di Consistenza Energetica
Per verificare la correttezza dell'integrazione:

```
E_cinetica_decollo = ½mv²_decollo
E_potenziale_apice = mgh_aria_calcolata
Errore_energetico = |E_cinetica - E_potenziale| / E_cinetica

Se Errore_energetico > 0.01 (1%):
    ⚠️ Possibili errori di integrazione
```

#### Test di Convergenza Numerica
Verifica della stabilità al variare di Δt:

```
Per Δt_test = [Δt, Δt/2, Δt/4]:
    h_test = calcola_altezza(Δt_test)
    
convergenza = |h_test[Δt] - h_test[Δt/2]| / |h_test[Δt/2] - h_test[Δt/4]|

Se convergenza > 4:
    ✅ Convergenza quadratica (metodo trapezi)
Se convergenza ≈ 2:
    ⚠️ Convergenza lineare (metodo Eulero)
```

#### Test di Simmetria Temporale (per BDI)
Verifica che l'inversione temporale funzioni correttamente:

```
v_decollo_FDI = FDI(dati_spinta).v_decollo
v_decollo_BDI = BDI(dati_atterraggio).v_ricostruita

Errore_simmetria = |v_decollo_FDI - v_decollo_BDI| / v_decollo_FDI

Se Errore_simmetria < 0.05 (5%):
    ✅ Simmetria temporale verificata
```

### Ottimizzazione Numerica

#### Adattamento Dinamico del Passo
Per dati non uniformemente campionati:

```
Δt[i] = t[i] - t[i-1]
v[i] = v[i-1] + a[i] × Δt[i]
s[i] = s[i-1] + v[i] × Δt[i]
```

#### Filtro Numerico Integrato
Per ridurre la propagazione del rumore:

```
a_filtrata[i] = α × a[i] + (1-α) × a_filtrata[i-1]
```
con α = 0.9 ÷ 0.95 (fattore di smorzamento)

#### Compensazione degli Errori di Drift
Correzione del drift finale per garantire v_finale ≈ 0:

```
drift_v = v[N-1] / N
Per i = 1 a N:
    v_corretta[i] = v[i] - i × drift_v
    
drift_s = s[N-1] / N  
Per i = 1 a N:
    s_corretta[i] = s[i] - i × drift_s
```

### Limiti di Validità delle Formule Numeriche

#### Frequenza di Campionamento Minima
```
f_min = 10 × f_Nyquist_movimento ≥ 1000 Hz
```

#### Durata Massima di Integrazione
Per limitare l'accumulo di errori:
```
T_max_FDI = 2 secondi
T_max_BDI = 1 secondo (fase atterraggio più breve)
```

#### Precisione del Sensore
```
Risoluzione_forza ≤ 0.1% × peso_corporeo
Linearità ≤ 0.5% fondo scala
Rumore_RMS ≤ 0.05% × peso_corporeo
```

### Relazione tra Approcci Continui e Discreti

#### Teorema di Convergenza
```
lim(Δt→0) Metodo_Numerico = Soluzione_Analitica_Continua
```

Le formule discrete **convergono** alle soluzioni continue quando il passo temporale tende a zero, garantendo la validità teorica dell'approssimazione numerica.

#### Validazione Incrociata
```
|h_continua - h_discreta| / h_continua < ε_tolleranza
```
con ε_tolleranza = 0.001 (0.1%) per validazione dell'implementazione numerica.

### Relazione tra Approcci Continui e Discreti

#### Teorema di Convergenza
```
lim(Δt→0) Metodo_Numerico = Soluzione_Analitica_Continua
```

Le formule discrete **convergono** alle soluzioni continue quando il passo temporale tende a zero, garantendo la validità teorica dell'approssimazione numerica.

#### Validazione Incrociata
```
|h_continua - h_discreta| / h_continua < ε_tolleranza
```
con ε_tolleranza = 0.001 (0.1%) per validazione dell'implementazione numerica.

### Analisi dell'Errore di Discretizzazione

#### Errore di Troncamento Locale
Per l'integrazione numerica, l'errore commesso ad ogni passo è:

**Metodo di Eulero:**
```
ε_locale = O(Δt²) × f'(t)
```

**Metodo dei Trapezi:**
```
ε_locale = O(Δt³) × f''(t)
```

**Metodo di Simpson:**
```
ε_locale = O(Δt⁵) × f⁽⁴⁾(t)
```

#### Errore Globale Accumulato
Dopo N passi di integrazione:

**Eulero:**
```
ε_globale = O(N × Δt²) = O(T × Δt)
```

**Trapezi:**
```
ε_globale = O(N × Δt³) = O(T × Δt²)
```

dove T = N × Δt è la durata totale dell'integrazione.

#### Condizione di Convergenza
Per garantire convergenza alla soluzione continua:
```
Δt < min(stabilità, precisione)

Stabilità: Δt < 2/ω_max
Precisione: Δt < √(ε_desiderato × 12 / (T × max|f''(t)|))
```

### Esempi Numerici di Validazione

#### Esempio 1: Test su Funzione Analitica Nota
```
Funzione test: f(t) = A sin(ωt)
Integrale analitico: F(t) = -A/ω cos(ωt) + C

Per A = 1000 N, ω = 10 Hz, T = 1 s:
- Soluzione analitica: F(1) = -100 cos(10) + C
- Eulero (Δt = 0.001): Errore ≈ 0.05%
- Trapezi (Δt = 0.001): Errore ≈ 0.0001%
```

#### Esempio 2: Convergenza su Dati Reali di Salto
```
Dati: F(t) da CMJ reale, T = 0.8 s
Riferimento: Δt = 0.0001 s (soluzione "esatta")

Δt = 0.01 s:  Errore = 5.2%    (inaccettabile)
Δt = 0.005 s: Errore = 1.8%    (limite accettabilità)
Δt = 0.001 s: Errore = 0.1%    (eccellente)
Δt = 0.0005 s: Errore = 0.03%  (overkill)
```

### Ottimizzazione delle Performance Numeriche

#### Scelta Adaptiva del Metodo
```
Se T < 0.5 s E f_campionamento > 2000 Hz:
    Usa Simpson per massima precisione
Altrimenti Se precisione_richiesta > 0.1%:
    Usa Trapezi (compromesso ottimale)
Altrimenti:
    Usa Eulero (velocità massima)
```

#### Pre-condizionamento dei Dati
```
1. Rimozione offset DC: F_corr[i] = F[i] - mean(F_quiet)
2. Filtro anti-aliasing: F_filt = lowpass(F_corr, f_cutoff)
3. Decimazione se necessario: F_dec = decimate(F_filt, fattore)
```

#### Post-processing per Correzione Errori
```
1. Compensazione drift lineare:
   drift = (v[N] - v[0]) / N
   v_corr[i] = v[i] - i × drift

2. Applicazione vincoli fisici:
   Se v_finale > 0.1 m/s: ⚠️ Errore di integrazione
   Se s_finale < -0.01 m: ⚠️ Drift eccessivo
```

## Confronto Quantitativo delle Formulazioni

### Precisione Teorica vs Pratica

| Metodo | Formulazione | Errore Teorico | Errore Pratico* | Applicabilità |
|--------|--------------|----------------|-----------------|---------------|
| **FDI-Continuo** | ∫∫ a(t) dt dt | 0 (analitico) | N/A | Solo teorica |
| **FDI-Trapezi** | Σ trapezi | O(Δt²) | ±1mm | Raccomandato |
| **FDI-Eulero** | Σ rettangoli | O(Δt) | ±5mm | Sconsigliato |
| **BDI-Continuo** | ∫∫ a_inv(t) dt dt | 0 (analitico) | N/A | Solo teorica |
| **BDI-Trapezi** | Σ trapezi inv. | O(Δt²) | ±2-3mm | Raccomandato |
| **FT+C-Analitico** | gt²/8 + C | 0 (cinematica) | ±11mm | Campo |

*Con f = 1000 Hz, T = 1s, SNR = 60 dB

### Stabilità Numerica

#### Analisi degli Autovalori
Per il sistema discretizzato, la stabilità richiede:
```
|λ_max| < 1

dove λ sono gli autovalori della matrice di iterazione:
λ = 1 + Δt × σ_sistema
```

#### Criterio di Routh-Hurwitz Discreto
```
Per stabilità assoluta:
Δt < 2 / |parte_reale_massima(σ)|
```

Nel caso del salto verticale:
```
σ_max ≈ 100 rad/s (movimento umano)
Δt_max = 2/100 = 0.02 s
f_min = 50 Hz (molto conservativo)
```

### Raccomandazioni Finali per l'Implementazione

#### Scelta del Metodo di Integrazione
```
Per f ≥ 1000 Hz: Trapezi (ottimale)
Per f = 500-1000 Hz: Trapezi con controllo errore
Per f < 500 Hz: Solo FT+C (FDI/BDI inaffidabili)
```

#### Parametri di Controllo Qualità
```
Controllo_convergenza: |h_Δt - h_Δt/2| / h_Δt/2 < 0.01
Controllo_energia: |E_cin - E_pot| / E_cin < 0.01  
Controllo_drift: |v_finale| < 0.1 m/s
Controllo_simmetria: |v_FDI - v_BDI| / v_FDI < 0.05
```

#### Soglie di Accettabilità
```
Ricerca scientifica: Errore < 0.5mm
Monitoraggio atleti: Errore < 2mm  
Screening campo: Errore < 5mm
```

Questa formulazione completa integra rigorosamente sia gli aspetti teorici continui che quelli pratici discreti, fornendo tutti gli strumenti matematici necessari per implementare e validare correttamente i tre metodi di calcolo dell'altezza del salto. Dinamico del Passo
Per dati non uniformemente campionati:

```
Δt[i] = t[i] - t[i-1]
v[i] = v[i-1] + a[i] × Δt[i]
s[i] = s[i-1] + v[i] × Δt[i]
```

#### Filtro Numerico Integrato
Per ridurre la propagazione del rumore:

```
a_filtrata[i] = α × a[i] + (1-α) × a_filtrata[i-1]
```
con α = 0.9 ÷ 0.95 (fattore di smorzamento)

#### Compensazione degli Errori di Drift
Correzione del drift finale per garantire v_finale ≈ 0:

```
drift_v = v[N-1] / N
Per i = 1 a N:
    v_corretta[i] = v[i] - i × drift_v
    
drift_s = s[N-1] / N  
Per i = 1 a N:
    s_corretta[i] = s[i] - i × drift_s
```

### Limiti di Validità delle Formule Numeriche

#### Frequenza di Campionamento Minima
```
f_min = 10 × f_Nyquist_movimento ≥ 1000 Hz
```

#### Durata Massima di Integrazione
Per limitare l'accumulo di errori:
```
T_max_FDI = 2 secondi
T_max_BDI = 1 secondo (fase atterraggio più breve)
```

#### Precisione del Sensore
```
Risoluzione_forza ≤ 0.1% × peso_corporeo
Linearità ≤ 0.5% fondo scala
Rumore_RMS ≤ 0.05% × peso_corporeo
```

Queste formule numeriche discrete forniscono la base matematica rigorosa per implementare correttamente i tre metodi, garantendo precisione e stabilità numerica.

### Assunzioni del Metodo

1. **Movimento puramente balistico** durante il volo
2. **Posizione del CoM identica** al decollo e all'atterraggio
3. **Costante antropometrica valida** per tutti i livelli di intensità
4. **Trascurabilità** della resistenza dell'aria

### Vantaggi
- **Semplicità estrema**: richiede solo il tempo di volo
- **Versatilità**: applicabile a tutti i tipi di salto
- **Portabilità**: utilizzabile sul campo senza pedane di forza
- **Correzione antropometrica**: tiene conto del heel-lift

### Limitazioni
- **Precisione limitata**: variabilità ±11mm (5x superiore a FDI/BDI)
- **Errori sistematici**: sovrastima con flessione eccessiva all'atterraggio
- **Costante fissa**: non si adatta a diverse strategie di movimento
- **Nessuna informazione biomeccanica** dettagliata

## Confronto dei Metodi

### Precisione e Accuratezza (Dati Sperimentali)

| Metodo | Bias medio | SD del bias | R² | ICC | Applicabilità |
|--------|------------|-------------|-----|-----|---------------|
| **FDI** | -0.4 mm | 0.9 mm | 0.989 | 0.994 | CMJ, SJ |
| **BDI** | -0.1 mm | 1.2 mm | 0.983 | 0.995 | CMJ, SJ, DJ |
| **FT+C** | -0.4 mm | 2.3 mm | 0.939 | 0.954 | Tutti |

### Informazioni Fornite

| Metodo | Altezza Totale | Heel-Lift | Air Height | Analisi Fasi | Complessità |
|--------|----------------|-----------|------------|--------------|-------------|
| **FDI** | ✅ | ✅ | ✅ | ✅ | Alta |
| **BDI** | ✅ | ❌ | ❌ | ❌ | Alta |
| **FT+C** | ✅ | ⚠️* | ⚠️* | ❌ | Bassa |

*Stima antropometrica, non misurazione diretta

## Analisi Biomeccanica delle Fasi del Salto

### Le Cinque Fasi del Countermovement Jump

#### Fase 1: Quiet Standing
- **Durata**: 2-3 secondi
- **Caratteristiche**: F = peso_corporeo, v = 0, a = 0
- **Significato**: Baseline, preparazione neuromuscolare, stabilità posturale

#### Fase 2: Countermovement (Discesa)
- **Durata**: 200-400 ms
- **Caratteristiche**: F < peso_corporeo, v < 0, a < 0
- **Significato**: Pre-attivazione muscolare, accumulo energia elastica, ottimizzazione lunghezza-tensione

#### Fase 3: Transizione
- **Durata**: 50-150 ms
- **Caratteristiche**: F ≈ peso → F > peso, v < 0 → v > 0, a massima
- **Significato**: Inversione movimento, rilascio energia elastica, picco attivazione neurale

#### Fase 4: Propulsione (Spinta)
- **Durata**: 150-300 ms
- **Caratteristiche**: F > peso_corporeo, v > 0, a > 0 → a = 0
- **Significato**: Generazione potenza, estensione articolare, heel-lift, accelerazione corpo

#### Fase 5: Volo
- **Durata**: 300-600 ms
- **Caratteristiche**: F = 0, v_decollo → 0 → -v_atterraggio, a = -g
- **Significato**: Movimento balistico, conversione energetica, apice salto

### Significato Fisico delle Due Altezze

#### Heel-Lift Distance
- **Origine**: Fasi 2, 3, 4 (countermovement + transizione + propulsione)
- **Meccanismo**: Lavoro muscolare attivo contro gravità
- **Significato**: Coordinazione multi-articolare, mobilità, efficienza tecnica
- **Proporzione tipica**: 20-25% dell'altezza totale

#### Air Height  
- **Origine**: Fase 5 (volo)
- **Meccanismo**: Conversione energia cinetica → energia potenziale
- **Significato**: Efficacia generazione potenza, capacità esplosiva
- **Proporzione tipica**: 75-80% dell'altezza totale

### Relazione Fasi → Componenti Altezza

Solo il metodo **FDI** permette di analizzare il contributo di ogni fase alle componenti dell'altezza:

- **Countermovement**: Contributo negativo al heel-lift (preparazione)
- **Transizione**: Fattore di efficienza per entrambe le componenti
- **Propulsione**: Contributo principale al heel-lift + generazione velocità per air height
- **Volo**: Conversione diretta velocità → air height

## Requisiti Tecnici

### Frequenze di Campionamento
- **FDI/BDI**: Minimo 1000 Hz, raccomandato 2000 Hz
- **FT+C**: Minimo 200 Hz, accettabile 500 Hz

### Filtraggio dei Segnali
- **FDI**: Low-pass 50 Hz, high-pass 0.1 Hz
- **BDI**: Low-pass 30 Hz, high-pass 0.1 Hz  
- **FT+C**: Low-pass 10 Hz

### Calibrazione
- **Peso corporeo**: Precisione ±0.1% per FDI/BDI
- **Offset**: Azzeramento durante quiet standing
- **Linearità**: Verifica su range completo di forze

## Raccomandazioni per la Scelta del Metodo

### Per Ricerca Scientifica
1. **FDI** come gold standard per countermovement jump
2. **BDI** per drop jump o validazione incrociata
3. **Mai FT+C** quando disponibili metodi più precisi

### Per Monitoraggio Atleti
1. **BDI** per versatilità (tutti i tipi di salto)
2. **FDI** per analisi dettagliata componenti
3. **FT+C** solo per screening rapido

### Per Applicazioni Sul Campo
1. **FT+C** quando non disponibili pedane di forza
2. **BDI** con pedane portatili
3. **Validazione periodica** con FDI in laboratorio

## Considerazioni per l'Implementazione

### Gestione degli Errori
- **Controllo qualità dati**: Verifica quiet standing, saturazione sensori
- **Validazione incrociata**: Confronto tra metodi quando possibile
- **Soglie di accettabilità**: Definizione range normali per validazione

### Interpretazione Risultati
- **Range normali**: 15-45 cm (ricreativo), 30-60 cm (allenato), 45-80 cm (elite)
- **Proporzioni heel-lift/air**: 18-30% / 70-82%
- **Velocità decollo**: 1.8-4.0 m/s

### Limitazioni Comuni
- **Variabilità inter-individuale**: Strategie motorie diverse
- **Effetti dell'allenamento**: Cambiamenti nelle proporzioni componenti
- **Condizioni di test**: Influenza calzature, superficie, istruzioni
