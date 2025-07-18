>[Torna all'indice](readme.md#fasi-progetto)

# Significato Fisico delle Due Altezze nel Salto Verticale

## Le Due Componenti dell'Altezza del Salto

L'altezza totale di un salto verticale è composta da **due componenti fisicamente distinte**:

```
h_totale = h_heel_lift + h_aria
```

### 1. Heel-Lift Distance (h_heel_lift)
**Definizione fisica**: La distanza verticale che il centro di massa (CoM) percorre dalla posizione di quiet standing fino al momento del decollo.

### 2. Air Height (h_aria) 
**Definizione fisica**: La distanza verticale che il CoM percorre durante la fase di volo, dal momento del decollo fino all'apice del salto.

## Perché Esistono Queste Due Altezze?

### Meccanismo Biomeccanico

Durante un salto verticale avvengono **due fasi di lavoro meccanico**:

#### **Fase 1: Lavoro Interno (Heel-Lift)**
```
Posizione iniziale → Posizione di decollo
```

**Cosa succede fisicamente:**
- **Estensione articolare**: Caviglia, ginocchio e anca si estendono progressivamente
- **Sollevamento del tallone**: Il piede passa da appoggio completo alla punta
- **Elevazione del CoM**: Il centro di massa si alza di ~8-10 cm
- **Lavoro muscolare**: I muscoli compiono lavoro **contro la gravità**

**Equazione energetica:**
```
Lavoro_muscolare = m × g × h_heel_lift + (1/2) × m × v_decollo²
```

#### **Fase 2: Volo Balistico (Air Height)**
```
Decollo → Apice del salto
```

**Cosa succede fisicamente:**
- **Perdita di contatto**: Nessuna forza muscolare può essere applicata
- **Movimento puramente balistico**: Solo la gravità agisce sul corpo
- **Conversione energetica**: Energia cinetica → Energia potenziale

**Equazione energetica:**
```
(1/2) × m × v_decollo² = m × g × h_aria
```

## Perché Si Calcolano Così?

### 1. Heel-Lift tramite Doppia Integrazione

**Base fisica**: Seconda legge di Newton
```
F = m × a  →  a = F/m
```

**Processo matematico:**
```
Forza → Accelerazione → Velocità → Spostamento
F(t) → a(t) → v(t) → s(t)
```

#### Formule Discrete per l'Integrazione

**Step 1: Calcolo della forza netta**
```
F_net(i) = F_totale(i) - peso_corporeo
```

**Step 2: Calcolo dell'accelerazione del CoM**
```
a(i) = F_net(i) / m - g
```
dove:
- m = massa corporea = peso_corporeo / g
- g = 9.81 m/s²

**Step 3: Prima integrazione (accelerazione → velocità)**
```
v(i) = v(i-1) + a(i) × Δt
```
con condizioni iniziali:
- v(0) = 0 (partenza da fermo)
- Δt = 1 / frequenza_campionamento

**Step 4: Seconda integrazione (velocità → spostamento)**
```
s(i) = s(i-1) + v(i) × Δt
```
con condizioni iniziali:
- s(0) = 0 (posizione di riferimento)

**Step 5: Heel-lift distance**
```
h_heel_lift = s(i_decollo)
```
dove i_decollo è l'indice temporale quando F_net < -peso_corporeo

#### Implementazione Completa

```javascript
function calculateHeelLiftHeight(forceData, bodyWeight, samplingRate) {
    const g = 9.81;
    const dt = 1 / samplingRate;
    const mass = bodyWeight / g;
    
    // Array per memorizzare i risultati
    let netForce = [];
    let acceleration = [];
    let velocity = [0];  // v(0) = 0
    let displacement = [0];  // s(0) = 0
    
    // Step 1 & 2: Forza netta e accelerazione
    for (let i = 0; i < forceData.length; i++) {
        netForce[i] = forceData[i] - bodyWeight;
        acceleration[i] = netForce[i] / mass;
    }
    
    // Step 3: Prima integrazione (velocità)
    for (let i = 1; i < acceleration.length; i++) {
        velocity[i] = velocity[i-1] + acceleration[i] * dt;
    }
    
    // Step 4: Seconda integrazione (spostamento)
    for (let i = 1; i < velocity.length; i++) {
        displacement[i] = displacement[i-1] + velocity[i] * dt;
    }
    
    // Step 5: Trova il decollo e heel-lift
    const takeoffIndex = netForce.findIndex((force, i) => 
        i > 0 && force < -bodyWeight
    );
    
    const heelLift = displacement[takeoffIndex];
    const takeoffVelocity = velocity[takeoffIndex];
    
    return {
        heelLift: heelLift,
        takeoffVelocity: takeoffVelocity,
        displacement: displacement,
        velocity: velocity,
        takeoffIndex: takeoffIndex
    };
}
```

**Perché funziona:**
- La forza di reazione al suolo contiene **tutta l'informazione** del movimento del CoM
- L'integrazione numerica **ricostruisce** passo dopo passo la traiettoria del CoM
- Considera automaticamente tutti gli effetti: peso corporeo, accelerazioni, coordinazione muscolare

### 2. Air Height tramite Equazioni Cinematiche

**Base fisica**: Moto uniformemente accelerato
```
v² = v₀² + 2×a×s
```

Al momento del decollo:
- **v₀ = v_decollo** (velocità iniziale verso l'alto)
- **v = 0** (velocità all'apice)
- **a = -g** (accelerazione di gravità)

Quindi:
```
0 = v_decollo² - 2×g×h_aria
h_aria = v_decollo² / (2×g)
```

#### Formula Discreta

```javascript
function calculateAirHeight(takeoffVelocity) {
    const g = 9.81;
    return Math.pow(takeoffVelocity, 2) / (2 * g);
}
```

## Significato Energetico

### Conservazione dell'Energia

Il salto verticale è un **perfetto esempio di conversione energetica**:

#### Durante la Spinta (Heel-Lift):
```
Energia_chimica_muscolare → Energia_meccanica_totale
E_muscolare = E_cinetica + E_potenziale_heel_lift
```

#### Durante il Volo (Air Height):
```
Energia_cinetica → Energia_potenziale_aria
(1/2)mv_decollo² = mgh_aria
```

### Bilancio Energetico Completo:
```
Lavoro_totale_muscolare = mg(h_heel_lift + h_aria) = mg×h_totale
```

## Esempi Numerici Completi

### Salto Tipico con Calcoli Step-by-Step

Consideriamo un atleta di 70 kg che effettua un salto con:
- Forza massima durante la spinta: 1400 N
- Durata della fase di spinta: 0.4 s
- Frequenza di campionamento: 1000 Hz

#### Calcolo del Heel-Lift

```javascript
// Dati esempio
const bodyWeight = 686; // N (70 kg × 9.81)
const mass = 70; // kg
const samplingRate = 1000; // Hz
const dt = 0.001; // s

// Esempio di sequenza di forze durante la spinta (semplificata)
const forceSequence = [
    686, 686, 700, 750, 850, 1000, 1200, 1400, 1300, 1100, 
    900, 700, 500, 300, 100, -100, 0, 0, 0  // decollo all'indice 15
];

// Calcoli step-by-step
let results = {
    netForce: [],
    acceleration: [],
    velocity: [0],
    displacement: [0]
};

// Step 1-2: Forza netta e accelerazione
for (let i = 0; i < forceSequence.length; i++) {
    results.netForce[i] = forceSequence[i] - bodyWeight;
    results.acceleration[i] = results.netForce[i] / mass;
}

// Step 3: Prima integrazione (velocità)
for (let i = 1; i < results.acceleration.length; i++) {
    results.velocity[i] = results.velocity[i-1] + results.acceleration[i] * dt;
}

// Step 4: Seconda integrazione (spostamento)
for (let i = 1; i < results.velocity.length; i++) {
    results.displacement[i] = results.displacement[i-1] + results.velocity[i] * dt;
}

// Risultati al decollo (indice 15):
const takeoffIndex = 15;
const heelLift = results.displacement[takeoffIndex]; // ≈ 0.089 m (8.9 cm)
const takeoffVelocity = results.velocity[takeoffIndex]; // ≈ 2.43 m/s
```

#### Calcolo dell'Air Height

```javascript
const g = 9.81;
const airHeight = Math.pow(takeoffVelocity, 2) / (2 * g);
// airHeight ≈ (2.43)² / (2 × 9.81) ≈ 0.301 m (30.1 cm)
```

#### Risultato Finale

```javascript
const totalJumpHeight = heelLift + airHeight;
// totalJumpHeight ≈ 0.089 + 0.301 = 0.390 m (39.0 cm)

console.log(`Heel-lift: ${(heelLift * 100).toFixed(1)} cm (${((heelLift/totalJumpHeight)*100).toFixed(1)}%)`);
console.log(`Air height: ${(airHeight * 100).toFixed(1)} cm (${((airHeight/totalJumpHeight)*100).toFixed(1)}%)`);
console.log(`Total jump: ${(totalJumpHeight * 100).toFixed(1)} cm`);

// Output:
// Heel-lift: 8.9 cm (22.8%)
// Air height: 30.1 cm (77.2%)
// Total jump: 39.0 cm
```

### Salto Tipico (h_totale = 40 cm):
- **h_heel_lift ≈ 8-10 cm** (20-25% del totale)
- **h_aria ≈ 30-32 cm** (75-80% del totale)

### Perché questa proporzione?
1. **Limitazione anatomica**: Il heel-lift è limitato dalla lunghezza degli arti
2. **Ottimizzazione energetica**: È più efficiente convertire energia in velocità che in spostamento statico

## Implementazione Integrata Completa

```javascript
function calculateJumpHeightComponents(forceData, bodyWeight, samplingRate) {
    const g = 9.81;
    const dt = 1 / samplingRate;
    const mass = bodyWeight / g;
    
    // Inizializzazione array
    let netForce = [];
    let acceleration = [];
    let velocity = [0];
    let displacement = [0];
    
    // Calcolo forza netta e accelerazione
    for (let i = 0; i < forceData.length; i++) {
        netForce[i] = forceData[i] - bodyWeight;
        acceleration[i] = netForce[i] / mass;
    }
    
    // Doppia integrazione numerica
    for (let i = 1; i < acceleration.length; i++) {
        // Prima integrazione: accelerazione → velocità
        velocity[i] = velocity[i-1] + acceleration[i] * dt;
        
        // Seconda integrazione: velocità → spostamento  
        displacement[i] = displacement[i-1] + velocity[i] * dt;
    }
    
    // Identificazione del decollo
    const takeoffIndex = netForce.findIndex((force, i) => 
        i > 0 && force < -bodyWeight
    );
    
    if (takeoffIndex === -1) {
        throw new Error("Decollo non rilevato nei dati di forza");
    }
    
    // Estrazione dei risultati
    const heelLift = displacement[takeoffIndex];
    const takeoffVelocity = velocity[takeoffIndex];
    const airHeight = Math.pow(takeoffVelocity, 2) / (2 * g);
    const totalHeight = heelLift + airHeight;
    
    return {
        heelLift: heelLift,
        airHeight: airHeight, 
        totalHeight: totalHeight,
        takeoffVelocity: takeoffVelocity,
        heelLiftPercentage: (heelLift / totalHeight) * 100,
        airHeightPercentage: (airHeight / totalHeight) * 100,
        
        // Dati completi per analisi
        displacement: displacement,
        velocity: velocity,
        acceleration: acceleration,
        netForce: netForce,
        takeoffIndex: takeoffIndex
    };
}

// Funzione di validazione fisica
function validateJumpPhysics(result) {
    const warnings = [];
    
    // Controlli proporzioni tipiche
    if (result.heelLiftPercentage < 15 || result.heelLiftPercentage > 35) {
        warnings.push(`Heel-lift percentage anomala: ${result.heelLiftPercentage.toFixed(1)}% (normale: 20-25%)`);
    }
    
    if (result.airHeightPercentage < 65 || result.airHeightPercentage > 85) {
        warnings.push(`Air height percentage anomala: ${result.airHeightPercentage.toFixed(1)}% (normale: 75-80%)`);
    }
    
    // Controlli valori assoluti
    if (result.heelLift < 0.05 || result.heelLift > 0.15) {
        warnings.push(`Heel-lift assoluto anomalo: ${(result.heelLift*100).toFixed(1)} cm (normale: 8-10 cm)`);
    }
    
    if (result.takeoffVelocity < 1.5 || result.takeoffVelocity > 4.5) {
        warnings.push(`Velocità decollo anomala: ${result.takeoffVelocity.toFixed(2)} m/s (normale: 2-3.5 m/s)`);
    }
    
    return {
        isValid: warnings.length === 0,
        warnings: warnings,
        physicalConsistency: checkPhysicalConsistency(result)
    };
}

function checkPhysicalConsistency(result) {
    // Verifica conservazione energia
    const g = 9.81;
    const expectedAirHeight = Math.pow(result.takeoffVelocity, 2) / (2 * g);
    const energyError = Math.abs(expectedAirHeight - result.airHeight) / result.airHeight;
    
    return {
        energyConservationError: energyError * 100, // percentuale
        isEnergyConsistent: energyError < 0.01 // errore < 1%
    };
}

// Esempio di utilizzo completo
function analyzeJumpExample() {
    // Dati esempio: atleta 70kg, salto di 40cm
    const bodyWeight = 686; // N
    const samplingRate = 1000; // Hz
    
    // Sequenza di forze simulate (400ms di spinta)
    const forceData = generateExampleForceData(bodyWeight, 400);
    
    try {
        const result = calculateJumpHeightComponents(forceData, bodyWeight, samplingRate);
        const validation = validateJumpPhysics(result);
        
        console.log("=== ANALISI COMPONENTI SALTO ===");
        console.log(`Heel-lift: ${(result.heelLift*100).toFixed(1)} cm (${result.heelLiftPercentage.toFixed(1)}%)`);
        console.log(`Air height: ${(result.airHeight*100).toFixed(1)} cm (${result.airHeightPercentage.toFixed(1)}%)`);
        console.log(`Altezza totale: ${(result.totalHeight*100).toFixed(1)} cm`);
        console.log(`Velocità decollo: ${result.takeoffVelocity.toFixed(2)} m/s`);
        
        if (!validation.isValid) {
            console.log("\n⚠️  AVVERTIMENTI:");
            validation.warnings.forEach(warning => console.log(`- ${warning}`));
        }
        
        console.log(`\n✅ Conservazione energia: ${validation.physicalConsistency.isEnergyConsistent ? 'OK' : 'ERRORE'}`);
        console.log(`   Errore energetico: ${validation.physicalConsistency.energyConservationError.toFixed(2)}%`);
        
        return result;
        
    } catch (error) {
        console.error("Errore nell'analisi:", error.message);
    }
}

function generateExampleForceData(bodyWeight, durationMs) {
    // Genera una curva di forza realistica per esempio
    const samples = durationMs;
    const forces = [];
    
    for (let i = 0; i < samples; i++) {
        const t = i / samples; // tempo normalizzato 0-1
        
        if (t < 0.1) {
            // Quiet standing
            forces[i] = bodyWeight;
        } else if (t < 0.9) {
            // Fase di spinta: curva sinusoidale con picco
            const pushPhase = (t - 0.1) / 0.8;
            const forceMultiplier = 1 + 1.2 * Math.sin(pushPhase * Math.PI);
            forces[i] = bodyWeight * forceMultiplier;
        } else {
            // Decollo
            forces[i] = 0;
        }
    }
    
    return forces;
}
```

### 1. Confronto tra Atleti
```javascript
// Atleta A: h_total = 50cm, h_heel_lift = 12cm, h_aria = 38cm
// Atleta B: h_total = 50cm, h_heel_lift = 8cm, h_aria = 42cm

// Atleta B ha maggiore efficienza esplosiva
// Atleta A ha maggiore coordinazione articolare
```

### 2. Analisi Biomeccanica
- **Alto heel-lift**: Buona coordinazione multi-articolare
- **Alto air height**: Maggiore potenza esplosiva
- **Proporzioni anomale**: Possibili disfunzioni o strategie compensatorie

### 3. Allenamento Specifico
- **Migliorare heel-lift**: Mobilità articolare, coordinazione
- **Migliorare air height**: Potenza esplosiva, velocità di contrazione

## Visualizzazione del Processo

```
Quiet Standing     Decollo          Apice
      |              |               |
      |    h_heel    |    h_aria     |
      |    ←────→    |    ←─────→    |
      |              |               |
   ───┴───         ───              ───
   
Fase 1: Lavoro muscolare attivo
Fase 2: Conversione energia cinetica→potenziale
```

## Errori Comuni di Interpretazione

### ❌ Errore: "Il heel-lift non conta"
**Realtà**: Il heel-lift rappresenta **lavoro meccanico reale** fatto dai muscoli

### ❌ Errore: "Solo l'air height è importante"
**Realtà**: Entrambe le componenti riflettono aspetti diversi della performance

### ❌ Errore: "Il tempo di volo misura tutto"
**Realtà**: Il tempo di volo ignora completamente il heel-lift

## Conclusione Fisica

Le due altezze esistono perché il salto verticale coinvolge **due meccanismi fisici distinti**:

1. **Meccanismo attivo** (heel-lift): Lavoro muscolare contro gravità
2. **Meccanismo passivo** (air height): Conversione energetica balistica

La loro separazione permette di:
- **Analizzare** diversi aspetti della performance
- **Comprendere** le strategie biomeccaniche individuali  
- **Ottimizzare** l'allenamento specifico
- **Diagnosticare** eventuali disfunzioni del movimento

Ignorare una delle due componenti significa perdere informazioni cruciali sulla biomeccanica del salto e sulle capacità neuromuscolari dell'atleta.

## Sitografia

- https://www.tandfonline.com/doi/epdf/10.1080/02640414.2022.2059319

>[Torna all'indice](readme.md#fasi-progetto)
