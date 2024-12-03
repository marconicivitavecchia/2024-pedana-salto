>[Torna all'indice](readme.md#fasi-progetto)

# Metodi di Rilevamento per Analisi del Salto

## 1. Metodo della Derivata del Segnale

### Calcolo Derivata
$$\dot{F}(t) = \frac{F_{\text{pedana}}(t_{i+1}) - F_{\text{pedana}}(t_i)}{\Delta t}$$

Nel dominio discreto:
$$\dot{F}[n] = \frac{F_{\text{pedana}}[n+1] - F_{\text{pedana}}[n]}{\Delta t}$$

### Soglie
- **Inizio salto**: $\dot{F}(t) > \text{soglia}_{\text{derivata}}$
- **Stacco**: $F_{\text{pedana}}(t) < 5\% \text{ del peso statico}$
- **Attacco**: $F_{\text{pedana}}(t)$ aumenta sopra 5% del peso statico
- **Fine caduta**: $\dot{F}(t) \approx 0$

## 2. Metodo Statistico

### Calcolo Peso Statico
$$F_{\text{statico}} = \frac{1}{N} \sum_{n=1}^N F_{\text{pedana}}[n]$$

### Definizione Eventi
- **Inizio salto**: $F_{\text{pedana}}[n] > F_{\text{statico}} + \text{soglia}_{\text{statica}}$
- **Stacco**: $F_{\text{pedana}}[n] < \text{soglia}_{\text{bassa}}$ ($\approx 5\%$ di $F_{\text{statico}}$)
- **Attacco**: $F_{\text{pedana}}[n] > \text{soglia}_{\text{bassa}}$
- **Fine caduta**: $F_{\text{pedana}}[n] \approx F_{\text{statico}}$

## 3. Metodo del Filtro Adattivo

### Media Mobile
$$\text{Media}_n = \frac{1}{w} \sum_{i=n-w}^n F_{\text{pedana}}[i]$$

### Calcolo Deviazioni
$$\Delta F = F_{\text{pedana}}(t) - \text{Media}_n$$

### Eventi
- **Inizio salto**: $\Delta F > \text{soglia}_{\text{positivo}}$
- **Stacco**: $\Delta F < -\text{soglia}_{\text{negativo}}$
- **Attacco**: $\Delta F > \text{soglia}_{\text{positivo}}$ dopo zero-crossing
- **Fine caduta**: $\Delta F \approx 0$

## 4. Metodo dell'Energia

### Energia Istantanea
$$E(t) = F_{\text{pedana}}(t)^2$$

### Eventi
- **Inizio salto**: Picco positivo di $\dot{E}(t)$
- **Stacco**: Minimo locale di $E(t)$
- **Attacco**: Picco positivo dopo minimo
- **Fine caduta**: $E(t)$ stabilizzata

# 5. Metodo Machine Learning

## Applicazione
- Dataset significativo di salti esistenti
- Analisi avanzata con modelli predittivi

## Feature Extraction
$$\text{Features} = \{F_{\text{pedana}}(t), \dot{F}(t), \sigma_F, \text{peaks}, \text{crossings}\}$$

## Eventi da Classificare
- $t_{\text{inizio}}$: Inizio salto
- $t_{\text{stacco}}$: Stacco dalla pedana
- $t_{\text{attacco}}$: Contatto con pedana
- $t_{\text{fine}}$: Fine movimento

## Caratteristiche
### Pro
- Adattabile a scenari diversi
- Riconosce pattern complessi

### Contro
- Training iniziale necessario
- Alto costo computazionale

## Uso Ottimale
Studi di ricerca e sport di alto livello con requisiti di analisi personalizzata

## Confronto Metodi

| Metodo | Vantaggi | Svantaggi |
|--------|----------|-----------|
| Derivata | Preciso per transizioni rapide | Sensibile al rumore |
| Statistico | Robusto, semplice | Meno adatto a movimenti complessi |
| Filtro adattivo | Migliore stabilità | Richiede regolazione fine |
| Energia | Facile implementazione | Sensibile a picchi accidentali |
| Machine Learning | Adattabile a scenari diversi, riconosce pattern complessi | Alto costo computazionale, richiede training |

>[Torna all'indice](readme.md#fasi-progetto)
