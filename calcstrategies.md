>[Torna all'indice](readme.md#fasi-progetto)

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

### Formule Matematiche

Le formule matematiche sono **identiche** al FDI, applicate ai dati temporalmente invertiti:

```
dati_invertiti = reverse(dati_atterraggio)
h_salto = FDI(dati_invertiti)
```

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

### Formule Matematiche

#### Altezza dal tempo di volo:
```
h_volo = g × t² / 8
```

#### Costante antropometrica (Wade et al., 2020):
```
C = (0.88 × LunghezzaPiede) + SpessoreSuola - AltezzaCaviglia
```

#### Altezza totale:
```
h_totale = h_volo + C
```

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

[Esempi di calcolo in JS](metodisaltojs.md)

## Sitografia

- https://www.tandfonline.com/doi/epdf/10.1080/02640414.2022.2059319

>[Torna all'indice](readme.md#fasi-progetto)
