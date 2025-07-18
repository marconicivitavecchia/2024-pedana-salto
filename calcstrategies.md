>[Torna all'indice](readme.md#fasi-progetto)

# Metodi FDI, BDI e FT+C per il Calcolo dell'Altezza del Salto

## Introduzione

Questo documento descrive tre metodi principali per calcolare l'altezza del salto verticale utilizzando dati di forza e tempo di volo, con le formule matematiche discrete necessarie per l'implementazione pratica.

## 1. Forward Double Integration (FDI)

### Principio
Il metodo FDI calcola l'altezza del salto integrando due volte i dati di forza durante la fase di spinta. Considera tutto il lavoro svolto per spingere il centro di massa (CoM) in aria.

### Formule Matematiche Discrete

#### Calcolo dell'accelerazione del CoM:
```
a(i) = (F_net(i) / m) - g
```
dove:
- F_net(i) = F_totale(i) - peso_corporeo
- m = massa corporea
- g = 9.81 m/s²

#### Prima integrazione (velocità):
```
v(i) = v(i-1) + a(i) × Δt
```
con v(0) = 0 (condizione iniziale)

#### Seconda integrazione (spostamento):
```
s(i) = s(i-1) + v(i) × Δt
```
con s(0) = 0 (condizione iniziale)

#### Distanza percorsa in aria:
```
h_aria = v_decollo² / (2 × g)
```

#### Altezza totale del salto:
```
h_totale = h_heel_lift + h_aria
```
dove h_heel_lift = s(i_decollo)

### Implementazione JavaScript

```javascript
function calculateFDI(forceData, bodyWeight, samplingRate) {
    const g = 9.81;
    const dt = 1 / samplingRate;
    const mass = bodyWeight / g;
    
    // Calcola la forza netta
    const netForce = forceData.map(force => force - bodyWeight);
    
    // Calcola accelerazione
    const acceleration = netForce.map(force => force / mass);
    
    // Prima integrazione: velocità
    let velocity = [0];
    for (let i = 1; i < acceleration.length; i++) {
        velocity[i] = velocity[i-1] + acceleration[i] * dt;
    }
    
    // Seconda integrazione: spostamento
    let displacement = [0];
    for (let i = 1; i < velocity.length; i++) {
        displacement[i] = displacement[i-1] + velocity[i] * dt;
    }
    
    // Trova l'indice di decollo (forza netta diventa negativa)
    const takeoffIndex = netForce.findIndex((force, i) => 
        i > 0 && force < -bodyWeight
    );
    
    // Calcola altezza del salto
    const heelLift = displacement[takeoffIndex];
    const takeoffVelocity = velocity[takeoffIndex];
    const airHeight = Math.pow(takeoffVelocity, 2) / (2 * g);
    const jumpHeight = heelLift + airHeight;
    
    return {
        jumpHeight: jumpHeight,
        heelLift: heelLift,
        airHeight: airHeight,
        takeoffVelocity: takeoffVelocity
    };
}
```

## 2. Backward Double Integration (BDI)

### Principio
Il metodo BDI utilizza la stessa matematica del FDI ma applica l'integrazione durante la fase di atterraggio. Il salto inizia e finisce in quiet standing, quindi tutta l'energia generata deve essere attenuata durante l'atterraggio.

### Il Problema delle Condizioni Iniziali

**Perché serve l'inversione temporale:**

L'integrazione numerica richiede sempre condizioni iniziali note:
```
v(i) = v(i-1) + a(i) × Δt  →  Serve v(0) noto!
s(i) = s(i-1) + v(i) × Δt  →  Serve s(0) noto!
```

#### FDI: Condizioni iniziali note ✅
```
Al quiet standing iniziale:
- v(0) = 0 (corpo fermo)
- s(0) = 0 (posizione di riferimento)
- F(0) = peso_corporeo
```

#### BDI: Problema senza inversione ❌
```
Al momento dell'impatto:
- v(0) = ??? (velocità di atterraggio sconosciuta)
- s(0) = ??? (altezza di atterraggio sconosciuta)
- F(0) = forza_impatto (noto)
```

#### BDI: Soluzione con inversione ✅
```
Invertendo i dati, il quiet standing finale diventa l'inizio:
- v(0) = 0 (corpo fermo nel quiet standing)
- s(0) = 0 (posizione di riferimento)
- F(0) = peso_corporeo
```

### Formule Matematiche Discrete

#### Step 1: Identificazione della fase di atterraggio
```
inizio_atterraggio: F(i) > 2 × peso_corporeo (impatto)
fine_atterraggio: |F(i) - peso_corporeo| < 0.1 × peso_corporeo per t > 0.5s
```

#### Step 2: Inversione temporale dei dati
```
dati_originali = [F_impatto, F_picco, F_decel, F_quiet]
dati_invertiti = [F_quiet, F_decel, F_picco, F_impatto]
```

#### Step 3: Integrazione normale sui dati invertiti
Le formule sono identiche al FDI, applicate ai dati temporalmente invertiti:

```
a(i) = (F_invertiti(i) / m) - g
v(i) = v(i-1) + a(i) × Δt    (con v(0) = 0)
s(i) = s(i-1) + v(i) × Δt    (con s(0) = 0)
```

#### Step 4: Estrazione dell'altezza del salto
```
h_salto = max(s(i))
```

### Implementazione JavaScript

```javascript
function calculateBDI(forceData, bodyWeight, samplingRate) {
    const g = 9.81;
    const dt = 1 / samplingRate;
    const mass = bodyWeight / g;
    
    // Step 1: Identifica la fase di atterraggio
    const landingStart = findLandingStart(forceData, bodyWeight);
    const landingEnd = findLandingEnd(forceData, bodyWeight, landingStart);
    
    // Step 2: Estrai i dati di atterraggio
    const landingForces = forceData.slice(landingStart, landingEnd);
    
    // Step 3: INVERSIONE TEMPORALE - Punto chiave!
    const reversedForces = [...landingForces].reverse();
    
    // Step 4: Calcola accelerazione sui dati invertiti
    const acceleration = reversedForces.map(force => (force - bodyWeight) / mass);
    
    // Step 5: Doppia integrazione normale sui dati invertiti
    let velocity = [0];      // Condizione iniziale nota: quiet standing
    let displacement = [0];  // Condizione iniziale nota: posizione riferimento
    
    for (let i = 1; i < acceleration.length; i++) {
        velocity[i] = velocity[i-1] + acceleration[i] * dt;
        displacement[i] = displacement[i-1] + velocity[i] * dt;
    }
    
    // Step 6: Estrai risultati
    const jumpHeight = Math.max(...displacement);
    const reconstructedTakeoffVelocity = Math.abs(velocity[velocity.length - 1]);
    
    return {
        jumpHeight: jumpHeight,
        reconstructedTakeoffVelocity: reconstructedTakeoffVelocity,
        landingPhaseData: {
            displacement: displacement,
            velocity: velocity,
            acceleration: acceleration,
            reversedForces: reversedForces
        }
    };
}

function findLandingStart(forceData, bodyWeight) {
    // Rileva l'impatto: forza > 2x peso corporeo
    return forceData.findIndex(force => force > 2 * bodyWeight);
}

function findLandingEnd(forceData, bodyWeight, startIndex) {
    const tolerance = bodyWeight * 0.1;
    
    // Cerca una finestra di quiet standing (forza stabile ≈ peso corporeo)
    for (let i = startIndex + 100; i < forceData.length - 50; i++) {
        const windowAvg = forceData.slice(i, i + 50)
            .reduce((sum, val) => sum + val, 0) / 50;
            
        if (Math.abs(windowAvg - bodyWeight) < tolerance) {
            return i;
        }
    }
    return forceData.length - 1;
}
```

### Visualizzazione del Processo BDI

```javascript
function demonstrateBDI_Process() {
    console.log("=== DIMOSTRAZIONE PROCESSO BDI ===\n");
    
    // Esempio: sequenza di atterraggio
    const landingSequence = [
        { time: 0.001, force: 100,  phase: "primo_contatto" },
        { time: 0.002, force: 1800, phase: "impatto" }, 
        { time: 0.003, force: 2500, phase: "picco_forza" },
        { time: 0.004, force: 1200, phase: "decelerazione" },
        { time: 0.005, force: 686,  phase: "quiet_standing" }
    ];
    
    console.log("DATI ORIGINALI (ordine temporale naturale):");
    landingSequence.forEach(point => {
        console.log(`t=${point.time}s: F=${point.force}N (${point.phase})`);
    });
    
    // Problema senza inversione
    console.log("\n❌ Senza inversione - condizioni iniziali sconosciute:");
    console.log(`t=0.001s: F=${landingSequence[0].force}N, v=???, s=???`);
    console.log("→ Impossibile integrare!");
    
    // Soluzione con inversione
    const reversed = [...landingSequence].reverse();
    console.log("\nDATI INVERTITI per BDI:");
    reversed.forEach((point, i) => {
        console.log(`step ${i}: F=${point.force}N (era ${point.phase})`);
    });
    
    console.log("\n✅ Con inversione - condizioni iniziali note:");
    console.log(`step 0: F=${reversed[0].force}N, v=0, s=0 (quiet standing)`);
    console.log("→ Integrazione possibile!");
    
    // Integrazione simulata
    const bodyWeight = 686;
    const mass = 70;
    const dt = 0.001;
    
    let velocity = [0];
    let displacement = [0];
    
    console.log("\nINTEGRAZIONE STEP-BY-STEP:");
    for (let i = 1; i < reversed.length; i++) {
        const netForce = reversed[i].force - bodyWeight;
        const acceleration = netForce / mass;
        
        velocity[i] = velocity[i-1] + acceleration * dt;
        displacement[i] = displacement[i-1] + velocity[i] * dt;
        
        console.log(`Step ${i}: F_net=${netForce}N → a=${acceleration.toFixed(1)}m/s² → v=${velocity[i].toFixed(4)}m/s → s=${displacement[i].toFixed(5)}m`);
    }
    
    const jumpHeight = Math.max(...displacement);
    console.log(`\n🎯 ALTEZZA SALTO: ${(jumpHeight*100).toFixed(1)} cm`);
    
    return jumpHeight;
}
```
```

## 3. Flight Time + Constant (FT+C)

### Principio
Il metodo FT+C combina il calcolo basico del tempo di volo con una costante antropometrica per compensare il heel-lift.

### Formule Matematiche Discrete

#### Altezza base dal tempo di volo:
```
h_base = (g × t²) / 8
```

#### Costante antropometrica (da Wade et al., 2020):
```
C = (0.88 × LunghezzaPiede) + SpessoreSuola - AltezzaCaviglia
```

#### Altezza totale del salto:
```
h_totale = h_base + C
```

### Implementazione JavaScript

```javascript
function calculateFTC(flightTime, footLength, soleThickness, ankleHeight) {
    const g = 9.81;
    
    // Calcolo altezza base dal tempo di volo
    const baseHeight = (g * Math.pow(flightTime, 2)) / 8;
    
    // Calcolo costante antropometrica
    const heelLiftConstant = (0.88 * footLength) + soleThickness - ankleHeight;
    
    // Altezza totale del salto
    const jumpHeight = baseHeight + heelLiftConstant;
    
    return {
        jumpHeight: jumpHeight,
        baseHeight: baseHeight,
        heelLiftConstant: heelLiftConstant,
        flightTime: flightTime
    };
}

function detectFlightTime(forceData, bodyWeight, samplingRate) {
    const threshold = bodyWeight * 0.1;
    let takeoffIndex = -1;
    let landingIndex = -1;
    
    // Trova decollo
    for (let i = 0; i < forceData.length; i++) {
        if (forceData[i] < threshold) {
            takeoffIndex = i;
            break;
        }
    }
    
    // Trova atterraggio
    for (let i = takeoffIndex + 1; i < forceData.length; i++) {
        if (forceData[i] > threshold) {
            landingIndex = i;
            break;
        }
    }
    
    if (takeoffIndex >= 0 && landingIndex > takeoffIndex) {
        return (landingIndex - takeoffIndex) / samplingRate;
    }
    
    return null;
}
```

## Parametri di Input e Misurazione

### Dati richiesti per FDI:
- Forze di reazione al suolo (1000 Hz minimo)
- Peso corporeo (calcolato da quiet standing)
- Frequenza di campionamento

### Dati richiesti per BDI:
- Forze di reazione al suolo (1000 Hz minimo)
- Peso corporeo
- Identificazione fase di atterraggio

### Dati richiesti per FT+C:
- Tempo di volo
- Lunghezza del piede (mm)
- Spessore della suola (mm)
- Altezza della caviglia (mm)

## Analisi Biomeccanica delle Fasi del Salto

### Le Cinque Fasi Fondamentali del Countermovement Jump

Un salto verticale completo può essere suddiviso in **cinque fasi distinte**, ciascuna con significato biomeccanico specifico:

```
Fase 1: Quiet Standing
Fase 2: Countermovement (Discesa)  
Fase 3: Transizione
Fase 4: Propulsione (Spinta)
Fase 5: Volo
```

### Fase 1: Quiet Standing
```
Durata: 2-3 secondi
Forza: F = peso_corporeo (costante)
Velocità: v = 0
Accelerazione: a = 0
```

**Significato biomeccanico:**
- **Baseline di riferimento**: Stabilisce il peso corporeo e la posizione di equilibrio
- **Preparazione neuromuscolare**: Attivazione del sistema nervoso centrale
- **Stabilità posturale**: Controllo dell'equilibrio statico

**Parametri di interesse:**
- Stabilità della forza (variabilità < 1% del peso corporeo)
- Durata adeguata per calcolo baseline
- Assenza di oscillazioni eccessive

### Fase 2: Countermovement (Discesa)
```
Durata: 200-400 ms
Forza: F < peso_corporeo (forza netta negativa)
Velocità: v < 0 (verso il basso, crescente in modulo)  
Accelerazione: a < 0 (decelerazione verso il basso)
```

**Significato biomeccanico:**
- **Pre-attivazione muscolare**: Stiramento dei muscoli estensori
- **Accumulo di energia elastica**: Deformazione di tendini e aponeurosi
- **Ottimizzazione lunghezza-tensione**: Posizionamento ottimale per la spinta
- **Coordinazione intersegmentale**: Sequenza ginocchio → anca → caviglia

**Parametri di interesse:**
```javascript
function analyzeCountermovement(forceData, velocity, displacement, bodyWeight) {
    const cmPhase = identifyCountermovementPhase(forceData, bodyWeight);
    
    return {
        duration: cmPhase.duration,                    // Durata del countermovement
        depth: Math.abs(Math.min(...displacement)),    // Profondità della discesa
        minForce: Math.min(...forceData),             // Forza minima raggiunta
        maxVelocityDown: Math.min(...velocity),       // Velocità massima verso il basso
        forceReduction: bodyWeight - Math.min(...forceData), // Riduzione di forza
        
        // Indici di performance
        cmDepthIndex: Math.abs(Math.min(...displacement)) * 100, // cm
        cmDurationIndex: cmPhase.duration * 1000,     // ms
        cmForceIndex: (bodyWeight - Math.min(...forceData)) / bodyWeight * 100 // %
    };
}
```

### Fase 3: Transizione
```
Durata: 50-150 ms
Forza: F ≈ peso_corporeo → F > peso_corporeo
Velocità: v < 0 → v = 0 → v > 0 (cambio direzione)
Accelerazione: a = 0 → a > 0 (massima accelerazione)
```

**Significato biomeccanico:**
- **Inversione del movimento**: Cambio da eccentrico a concentrico
- **Utilizzo energia elastica**: Rilascio dell'energia accumulata nei tendini
- **Massima attivazione neurale**: Picco di reclutamento delle unità motorie
- **Coupling time**: Tempo di accoppiamento eccentrico-concentrico

**Parametri di interesse:**
```javascript
function analyzeTransition(forceData, velocity, samplingRate) {
    const transitionPoint = velocity.findIndex(v => v === 0); // Cambio direzione
    const transitionStart = transitionPoint - Math.floor(0.1 * samplingRate); // 100ms prima
    const transitionEnd = transitionPoint + Math.floor(0.1 * samplingRate);   // 100ms dopo
    
    return {
        transitionTime: (transitionEnd - transitionStart) / samplingRate * 1000, // ms
        maxAcceleration: Math.max(...forceData.slice(transitionStart, transitionEnd).map(
            (f, i) => (f - bodyWeight) / mass
        )),
        forceAtTransition: forceData[transitionPoint],
        
        // Efficienza della transizione
        couplingTime: calculateCouplingTime(velocity), // Tempo tra v_min e v=0
        elasticUtilization: calculateElasticUtilization(forceData, transitionStart, transitionEnd)
    };
}
```

### Fase 4: Propulsione (Spinta)
```
Durata: 150-300 ms
Forza: F > peso_corporeo (forza netta positiva)
Velocità: v > 0 (verso l'alto, crescente)
Accelerazione: a > 0 → a = 0 (al decollo)
```

**Significato biomeccanico:**
- **Generazione di potenza**: Produzione massima di forza e velocità
- **Estensione articolare**: Sequenza caviglia → ginocchio → anca
- **Heel-lift**: Sollevamento del centro di massa
- **Accelerazione del corpo**: Conferimento dell'energia cinetica

**Parametri di interesse:**
```javascript
function analyzePropulsion(forceData, velocity, displacement, bodyWeight, samplingRate) {
    const propulsionPhase = identifyPropulsionPhase(forceData, velocity, bodyWeight);
    
    return {
        duration: propulsionPhase.duration,
        peakForce: Math.max(...forceData),
        averageForce: propulsionPhase.averageForce,
        takeoffVelocity: velocity[propulsionPhase.endIndex],
        heelLift: displacement[propulsionPhase.endIndex],
        
        // Indici di potenza
        peakPower: calculatePeakPower(forceData, velocity),
        averagePower: calculateAveragePower(forceData, velocity, propulsionPhase),
        forceVelocityProfile: analyzeForceVelocityRelation(forceData, velocity),
        
        // Efficienza
        mechanicalEfficiency: calculateMechanicalEfficiency(forceData, displacement, propulsionPhase),
        rateOfForceGeneration: calculateRFD(forceData, samplingRate)
    };
}

function calculatePeakPower(forceData, velocity) {
    let maxPower = 0;
    for (let i = 0; i < forceData.length; i++) {
        const power = forceData[i] * Math.max(0, velocity[i]); // Solo fase positiva
        if (power > maxPower) maxPower = power;
    }
    return maxPower;
}

function calculateRFD(forceData, samplingRate) {
    // Rate of Force Development nei primi 100ms della spinta
    const rfdWindow = Math.floor(0.1 * samplingRate); // 100ms
    const startIndex = forceData.findIndex(f => f > bodyWeight);
    
    if (startIndex + rfdWindow >= forceData.length) return null;
    
    const forceChange = forceData[startIndex + rfdWindow] - forceData[startIndex];
    const timeChange = rfdWindow / samplingRate;
    
    return forceChange / timeChange; // N/s
}
```

### Fase 5: Volo
```
Durata: 300-600 ms
Forza: F = 0 (nessun contatto)
Velocità: v_decollo → 0 → -v_atterraggio
Accelerazione: a = -g (solo gravità)
```

**Significato biomeccanico:**
- **Movimento balistico**: Solo la gravità agisce sul corpo
- **Conversione energetica**: Energia cinetica → energia potenziale → energia cinetica
- **Apice del salto**: Massima altezza raggiunta
- **Preparazione atterraggio**: Posizionamento per l'impatto

**Parametri di interesse:**
```javascript
function analyzeFlightPhase(flightTime, takeoffVelocity, landingVelocity) {
    const g = 9.81;
    
    return {
        flightTime: flightTime,
        airHeight: Math.pow(takeoffVelocity, 2) / (2 * g),
        timeToApex: takeoffVelocity / g,
        apexVelocity: 0, // Per definizione
        
        // Simmetria del volo
        flightSymmetry: Math.abs(takeoffVelocity - Math.abs(landingVelocity)) / takeoffVelocity,
        energyLoss: calculateAirResistanceLoss(takeoffVelocity, landingVelocity),
        
        // Preparazione atterraggio
        landingPreparation: analyzeLandingPreparation(flightTime)
    };
}
```

### Analisi Integrata delle Fasi

```javascript
function performCompleteJumpAnalysis(forceData, bodyWeight, samplingRate) {
    // Calcola velocità e spostamento tramite FDI
    const fdiResult = calculateFDI(forceData, bodyWeight, samplingRate);
    
    // Identifica le fasi
    const phases = identifyJumpPhases(forceData, fdiResult.velocity, bodyWeight);
    
    // Analizza ogni fase
    const analysis = {
        quietStanding: analyzeQuietStanding(forceData.slice(0, phases.quietEnd)),
        countermovement: analyzeCountermovement(
            forceData.slice(phases.cmStart, phases.cmEnd),
            fdiResult.velocity.slice(phases.cmStart, phases.cmEnd),
            fdiResult.displacement.slice(phases.cmStart, phases.cmEnd),
            bodyWeight
        ),
        transition: analyzeTransition(
            forceData.slice(phases.transStart, phases.transEnd),
            fdiResult.velocity.slice(phases.transStart, phases.transEnd),
            samplingRate
        ),
        propulsion: analyzePropulsion(
            forceData.slice(phases.propStart, phases.propEnd),
            fdiResult.velocity.slice(phases.propStart, phases.propEnd),
            fdiResult.displacement.slice(phases.propStart, phases.propEnd),
            bodyWeight,
            samplingRate
        ),
        flight: analyzeFlightPhase(
            phases.flightTime,
            fdiResult.takeoffVelocity,
            fdiResult.landingVelocity
        )
    };
    
    // Calcola indici sintetici
    analysis.summary = {
        totalJumpHeight: fdiResult.jumpHeight,
        jumpStrategy: classifyJumpStrategy(analysis),
        performanceProfile: generatePerformanceProfile(analysis),
        recommendations: generateRecommendations(analysis)
    };
    
    return analysis;
}

function classifyJumpStrategy(analysis) {
    const cmDepth = analysis.countermovement.depth;
    const cmDuration = analysis.countermovement.duration;
    const propulsionDuration = analysis.propulsion.duration;
    
    if (cmDepth > 0.15 && cmDuration > 0.3) {
        return "Deep countermovement strategy";
    } else if (cmDepth < 0.08 && propulsionDuration > 0.25) {
        return "Quick propulsion strategy";
    } else if (analysis.transition.couplingTime < 0.1) {
        return "Elastic utilization strategy";
    } else {
        return "Balanced strategy";
    }
}

function generatePerformanceProfile(analysis) {
    return {
        strengths: identifyStrengths(analysis),
        weaknesses: identifyWeaknesses(analysis),
        dominantQualities: analyzeDominantQualities(analysis)
    };
}

function identifyStrengths(analysis) {
    const strengths = [];
    
    if (analysis.propulsion.peakForce > analysis.bodyWeight * 2.5) {
        strengths.push("Elevata capacità di forza massima");
    }
    
    if (analysis.propulsion.rateOfForceGeneration > 3000) { // N/s
        strengths.push("Eccellente Rate of Force Development");
    }
    
    if (analysis.transition.couplingTime < 0.08) {
        strengths.push("Ottima utilizzazione dell'energia elastica");
    }
    
    if (analysis.flight.airHeight > 0.35) {
        strengths.push("Elevata conversione in altezza");
    }
    
    return strengths;
}
```

### Significato Clinico e Prestativo

#### Indicatori di Performance
```javascript
const performanceIndicators = {
    explosiveness: {
        metrics: ['peakPower', 'rateOfForceGeneration', 'takeoffVelocity'],
        normal: [2000, 3000, 2.5], // W, N/s, m/s
        elite: [4000, 5000, 3.5]
    },
    efficiency: {
        metrics: ['mechanicalEfficiency', 'couplingTime', 'flightSymmetry'],
        normal: [0.7, 0.12, 0.05], // %, s, %
        elite: [0.85, 0.08, 0.02]
    },
    coordination: {
        metrics: ['cmDepth', 'transitionTime', 'heelLiftPercentage'],
        normal: [0.12, 0.1, 22], // m, s, %
        elite: [0.08, 0.08, 20]
    }
};
```

#### Applicazioni Pratiche
1. **Monitoraggio allenamento**: Tracciamento dei miglioramenti fase-specifici
2. **Identificazione deficit**: Individuazione delle fasi limitanti la performance
3. **Prevenzione infortuni**: Analisi degli squilibri neuromuscolari
4. **Personalizzazione training**: Allenamento mirato alle fasi carenti

## Significato Fisico delle Due Altezze del Salto

### Le Due Componenti dell'Altezza Totale

L'altezza totale di un salto verticale è composta da **due componenti fisicamente distinte** che riflettono diversi aspetti della performance:

```
h_totale = h_heel_lift + h_aria
```

#### 1. Heel-Lift Distance (h_heel_lift)
**Definizione fisica**: La distanza verticale che il centro di massa (CoM) percorre dalla posizione di quiet standing fino al momento del decollo.

**Origine biomeccanica**: 
- Estensione progressiva di caviglia, ginocchio e anca
- Sollevamento del tallone (heel-lift)  
- Lavoro muscolare **attivo** contro la gravità
- Include il lavoro delle fasi 2, 3 e 4 (countermovement + transizione + propulsione)

#### 2. Air Height (h_aria)
**Definizione fisica**: La distanza verticale che il CoM percorre durante la fase di volo, dal momento del decollo fino all'apice del salto.

**Origine biomeccanica**:
- Conversione dell'energia cinetica in energia potenziale
- Movimento **puramente balistico** (solo gravità)
- Riflette l'efficacia della fase di propulsione
- Corrisponde alla fase 5 (volo)

### Relazione con le Fasi del Salto

```javascript
function relateHeightComponentsToPhases(jumpAnalysis) {
    const phaseContributions = {
        heel_lift: {
            countermovement: jumpAnalysis.countermovement.depth,        // Contributo negativo
            transition: jumpAnalysis.transition.displacement,          // Contributo variabile  
            propulsion: jumpAnalysis.propulsion.heelLift,             // Contributo principale
            total: jumpAnalysis.propulsion.heelLift - Math.abs(jumpAnalysis.countermovement.depth)
        },
        air_height: {
            propulsion_energy: jumpAnalysis.propulsion.takeoffVelocity,  // Energia conferita
            flight_conversion: jumpAnalysis.flight.airHeight,           // Conversione in altezza
            total: jumpAnalysis.flight.airHeight
        }
    };
    
    return {
        heel_lift_breakdown: phaseContributions.heel_lift,
        air_height_source: phaseContributions.air_height,
        phase_efficiency: calculatePhaseEfficiency(phaseContributions)
    };
}
```

### Calcolo delle Due Componenti con Analisi delle Fasi

```javascript
function calculateDetailedJumpComponents(forceData, bodyWeight, samplingRate) {
    // Analisi completa delle fasi
    const fullAnalysis = performCompleteJumpAnalysis(forceData, bodyWeight, samplingRate);
    
    // Calcolo delle componenti con breakdown delle fasi
    const components = {
        // Heel-lift dettagliato
        heel_lift: {
            total: fullAnalysis.propulsion.heelLift,
            countermovement_contribution: -Math.abs(fullAnalysis.countermovement.depth),
            propulsion_contribution: fullAnalysis.propulsion.heelLift + Math.abs(fullAnalysis.countermovement.depth),
            efficiency: fullAnalysis.propulsion.heelLift / fullAnalysis.propulsion.duration
        },
        
        // Air height dettagliato  
        air_height: {
            total: fullAnalysis.flight.airHeight,
            from_takeoff_velocity: Math.pow(fullAnalysis.propulsion.takeoffVelocity, 2) / (2 * 9.81),
            energy_source: fullAnalysis.propulsion.averagePower * fullAnalysis.propulsion.duration,
            conversion_efficiency: fullAnalysis.flight.airHeight / 
                (Math.pow(fullAnalysis.propulsion.takeoffVelocity, 2) / (2 * 9.81))
        },
        
        // Proporzioni e significato
        proportions: {
            heel_lift_percentage: (fullAnalysis.propulsion.heelLift / 
                (fullAnalysis.propulsion.heelLift + fullAnalysis.flight.airHeight)) * 100,
            air_height_percentage: (fullAnalysis.flight.airHeight / 
                (fullAnalysis.propulsion.heelLift + fullAnalysis.flight.airHeight)) * 100
        },
        
        // Interpretazione biomeccanica
        biomechanical_meaning: interpretComponentProportions(
            fullAnalysis.propulsion.heelLift, 
            fullAnalysis.flight.airHeight,
            fullAnalysis
        )
    };
    
    return components;
}

function interpretComponentProportions(heelLift, airHeight, fullAnalysis) {
    const heelLiftPerc = (heelLift / (heelLift + airHeight)) * 100;
    const interpretation = {
        coordination_quality: "Normal",
        power_profile: "Balanced", 
        technical_efficiency: "Standard",
        recommendations: []
    };
    
    // Analisi heel-lift
    if (heelLiftPerc < 15) {
        interpretation.coordination_quality = "Limited heel-lift";
        interpretation.recommendations.push("Migliorare mobilità articolare e coordinazione");
    } else if (heelLiftPerc > 30) {
        interpretation.coordination_quality = "Excessive heel-lift";
        interpretation.recommendations.push("Ottimizzare timing e efficienza del movimento");
    }
    
    // Analisi air height in relazione alla potenza
    const powerToHeightRatio = fullAnalysis.propulsion.peakPower / airHeight;
    if (powerToHeightRatio > 15000) { // W/m
        interpretation.power_profile = "High power, low conversion";
        interpretation.recommendations.push("Migliorare efficienza della propulsione");
    } else if (powerToHeightRatio < 8000) {
        interpretation.power_profile = "Efficient power conversion";
    }
    
    // Analisi efficienza tecnica
    const couplingTime = fullAnalysis.transition.couplingTime;
    if (couplingTime > 0.12) {
        interpretation.technical_efficiency = "Slow transition";
        interpretation.recommendations.push("Allenamento elastico e reattivo");
    } else if (couplingTime < 0.06) {
        interpretation.technical_efficiency = "Excellent elastic utilization";
    }
    
    return interpretation;
}
```

### Perché il BDI Non Fornisce le Due Componenti Separate

Il metodo BDI calcola direttamente l'altezza totale del salto ma **non può separare** heel-lift e air height:

```javascript
// BDI fornisce solo:
const bdiResult = {
    totalHeight: 0.40,     // Altezza totale (heel-lift + air height)
    // ❌ Non può fornire heel-lift separatamente
    // ❌ Non può fornire air height separatamente
    // ❌ Non può fornire analisi delle fasi
};

// Motivo fisico: BDI analizza solo la fase di atterraggio
// Non ha informazioni su:
// - Fase di countermovement (contributo al heel-lift)
// - Fase di transizione (efficienza del coupling)
// - Fase di propulsione (generazione heel-lift + velocità)
// - Separazione energia → heel-lift vs energia → velocità
```

### Integrazione con l'Analisi delle Fasi

```javascript
function integratePhaseAnalysisWithHeightComponents(forceData, bodyWeight, samplingRate) {
    // Analisi completa
    const phaseAnalysis = performCompleteJumpAnalysis(forceData, bodyWeight, samplingRate);
    const heightComponents = calculateDetailedJumpComponents(forceData, bodyWeight, samplingRate);
    
    // Mappa fase → componente altezza
    const phaseToHeightMapping = {
        countermovement: {
            effect_on: "heel_lift",
            contribution: "negative", // Abbassa il CoM
            magnitude: Math.abs(phaseAnalysis.countermovement.depth),
            biomechanical_role: "Preparazione e pre-attivazione"
        },
        
        transition: {
            effect_on: "heel_lift", 
            contribution: "minimal_direct", // Principalmente efficienza
            magnitude: "timing_dependent",
            biomechanical_role: "Efficienza coupling eccentrico-concentrico"
        },
        
        propulsion: {
            effect_on: "both",
            heel_lift_contribution: heightComponents.heel_lift.propulsion_contribution,
            air_height_contribution: "via_takeoff_velocity",
            magnitude: phaseAnalysis.propulsion.heelLift,
            biomechanical_role: "Generazione di potenza e altezza"
        },
        
        flight: {
            effect_on: "air_height",
            contribution: "total", // 100% dell'air height
            magnitude: heightComponents.air_height.total,
            biomechanical_role: "Conversione energia cinetica → potenziale"
        }
    };
    
    return {
        phase_analysis: phaseAnalysis,
        height_components: heightComponents,
        phase_height_mapping: phaseToHeightMapping,
        performance_insights: generatePerformanceInsights(phaseAnalysis, heightComponents)
    };
}

function generatePerformanceInsights(phaseAnalysis, heightComponents) {
    const insights = {
        dominant_phase: identifyDominantPhase(phaseAnalysis),
        limiting_factor: identifyLimitingFactor(phaseAnalysis, heightComponents),
        training_focus: suggestTrainingFocus(phaseAnalysis, heightComponents),
        technical_optimization: suggestTechnicalOptimizations(phaseAnalysis)
    };
    
    return insights;
}

function identifyDominantPhase(phaseAnalysis) {
    const phaseContributions = {
        countermovement: phaseAnalysis.countermovement.depth * 2, // Peso maggiore per CM profondo
        transition: 1 / phaseAnalysis.transition.couplingTime,    // Maggiore = migliore
        propulsion: phaseAnalysis.propulsion.peakPower / 1000,   // Normalizzato
        coordination: phaseAnalysis.summary.jumpStrategy
    };
    
    const maxContribution = Math.max(...Object.values(phaseContributions).filter(v => typeof v === 'number'));
    const dominantPhase = Object.entries(phaseContributions).find(([key, value]) => value === maxContribution)?.[0];
    
    return {
        phase: dominantPhase,
        strength: "high",
        implication: `Performance principalmente determinata dalla fase di ${dominantPhase}`
    };
}
```

### Esempio Numerico Completo con Analisi delle Fasi

```javascript
function demonstrateCompleteAnalysis() {
    console.log("=== ANALISI COMPLETA: FASI + COMPONENTI ALTEZZA ===\n");
    
    // Simula risultati di un salto reale
    const exampleResults = {
        phases: {
            countermovement: { depth: 0.12, duration: 0.3 },      // 12cm, 300ms
            transition: { couplingTime: 0.08, maxAccel: 25 },     // 80ms, 25m/s²
            propulsion: { duration: 0.2, heelLift: 0.095, takeoffVel: 2.8 }, // 200ms, 9.5cm, 2.8m/s
            flight: { airHeight: 0.31, flightTime: 0.57 }         // 31cm, 570ms
        },
        components: {
            heel_lift: 0.095,      // 9.5cm (25% del totale)
            air_height: 0.31,      // 31cm (75% del totale) 
            total: 0.405           // 40.5cm totale
        }
    };
    
    console.log("CONTRIBUTI DELLE FASI:");
    console.log(`Countermovement: -${(exampleResults.phases.countermovement.depth*100).toFixed(1)}cm (preparazione)`);
    console.log(`Transizione: Coupling time ${(exampleResults.phases.transition.couplingTime*1000).toFixed(0)}ms (efficienza)`);
    console.log(`Propulsione: +${(exampleResults.phases.propulsion.heelLift*100).toFixed(1)}cm heel-lift + ${exampleResults.phases.propulsion.takeoffVel.toFixed(1)}m/s velocità`);
    console.log(`Volo: ${(exampleResults.phases.flight.airHeight*100).toFixed(1)}cm da conversione energia cinetica`);
    
    console.log("\nCOMPONENTI ALTEZZA:");
    console.log(`Heel-lift: ${(exampleResults.components.heel_lift*100).toFixed(1)}cm (${((exampleResults.components.heel_lift/exampleResults.components.total)*100).toFixed(1)}%)`);
    console.log(`- Origine: Lavoro muscolare attivo durante propulsione`);
    console.log(`- Significato: Coordinazione multi-articolare e mobilità`);
    
    console.log(`Air height: ${(exampleResults.components.air_height*100).toFixed(1)}cm (${((exampleResults.components.air_height/exampleResults.components.total)*100).toFixed(1)}%)`);
    console.log(`- Origine: Conversione energia cinetica (v=${exampleResults.phases.propulsion.takeoffVel}m/s)`);
    console.log(`- Significato: Efficacia della generazione di potenza`);
    
    console.log(`\nTOTALE: ${(exampleResults.components.total*100).toFixed(1)}cm`);
    
    // Interpretazione integrata
    console.log("\nINTERPRETAZIONE INTEGRATA:");
    if (exampleResults.phases.countermovement.depth > 0.1) {
        console.log("✓ Buona utilizzazione del countermovement");
    }
    if (exampleResults.phases.transition.couplingTime < 0.1) {
        console.log("✓ Eccellente transizione eccentrico-concentrica");
    }
    if ((exampleResults.components.heel_lift/exampleResults.components.total) < 0.3) {
        console.log("✓ Proporzioni ottimali heel-lift/air-height");
    }
    
    return exampleResults;
}
```

### Esempio Pratico di Implementazione Completa

```javascript
// Sistema completo di analisi del salto
class JumpAnalyzer {
    constructor(samplingRate = 1000) {
        this.samplingRate = samplingRate;
        this.g = 9.81;
    }
    
    analyzeCompleteJump(forceData, bodyWeight, anthropometrics = null) {
        try {
            // 1. Analisi biomeccanica delle fasi
            const phaseAnalysis = this.performCompleteJumpAnalysis(forceData, bodyWeight);
            
            // 2. Calcolo altezze con metodi multipli
            const heightAnalysis = this.calculateAllHeightMethods(forceData, bodyWeight, anthropometrics);
            
            // 3. Integrazione fasi + altezze
            const integratedAnalysis = this.integrateAnalyses(phaseAnalysis, heightAnalysis);
            
            // 4. Generazione insights e raccomandazioni
            const insights = this.generateInsights(integratedAnalysis);
            
            return {
                phases: phaseAnalysis,
                heights: heightAnalysis,
                integrated: integratedAnalysis,
                insights: insights,
                report: this.generateReport(phaseAnalysis, heightAnalysis, insights)
            };
            
        } catch (error) {
            return this.handleAnalysisError(error, forceData, bodyWeight);
        }
    }
    
    calculateAllHeightMethods(forceData, bodyWeight, anthropometrics) {
        const results = {};
        
        // FDI - Metodo completo con componenti separate
        try {
            const fdi = calculateFDI(forceData, bodyWeight, this.samplingRate);
            results.FDI = {
                method: 'FDI',
                totalHeight: fdi.jumpHeight,
                heelLift: fdi.heelLift,
                airHeight: fdi.airHeight,
                takeoffVelocity: fdi.takeoffVelocity,
                components: {
                    heelLiftPercentage: (fdi.heelLift / fdi.jumpHeight) * 100,
                    airHeightPercentage: (fdi.airHeight / fdi.jumpHeight) * 100
                },
                confidence: 'High',
                applicability: 'Countermovement and Squat Jump'
            };
        } catch (error) {
            results.FDI = { error: error.message, confidence: 'Failed' };
        }
        
        // BDI - Metodo universale, solo altezza totale
        try {
            const bdi = calculateBDI(forceData, bodyWeight, this.samplingRate);
            results.BDI = {
                method: 'BDI',
                totalHeight: bdi.jumpHeight,
                heelLift: null, // Non disponibile
                airHeight: null, // Non disponibile
                reconstructedTakeoffVelocity: bdi.reconstructedTakeoffVelocity,
                components: null, // Non separabili
                confidence: 'High',
                applicability: 'All jump types'
            };
        } catch (error) {
            results.BDI = { error: error.message, confidence: 'Failed' };
        }
        
        // FT+C - Metodo semplificato
        if (anthropometrics) {
            try {
                const flightTime = this.detectFlightTime(forceData, bodyWeight);
                const ftc = calculateFTC(flightTime, anthropometrics.footLength, 
                                       anthropometrics.soleThickness, anthropometrics.ankleHeight);
                results.FTC = {
                    method: 'FT+C',
                    totalHeight: ftc.jumpHeight,
                    heelLift: ftc.heelLiftConstant, // Stima antropometrica
                    airHeight: ftc.baseHeight,
                    flightTime: ftc.flightTime,
                    components: {
                        heelLiftPercentage: (ftc.heelLiftConstant / ftc.jumpHeight) * 100,
                        airHeightPercentage: (ftc.baseHeight / ftc.jumpHeight) * 100
                    },
                    confidence: 'Medium',
                    applicability: 'Field applications'
                };
            } catch (error) {
                results.FTC = { error: error.message, confidence: 'Failed' };
            }
        }
        
        // Validazione incrociata
        results.validation = this.validateMethodAgreement(results);
        results.recommended = this.selectRecommendedMethod(results);
        
        return results;
    }
    
    integrateAnalyses(phaseAnalysis, heightAnalysis) {
        const integration = {
            phaseHeightMapping: {},
            performanceProfile: {},
            biomechanicalEfficiency: {},
            trainingImplications: {}
        };
        
        // Mappa fasi → componenti altezza (solo se FDI disponibile)
        if (heightAnalysis.FDI && !heightAnalysis.FDI.error) {
            integration.phaseHeightMapping = {
                countermovement: {
                    contribution_to_heel_lift: "Negative (preparation)",
                    depth: phaseAnalysis.countermovement.depth,
                    effect: "Pre-activation and energy storage"
                },
                transition: {
                    contribution_to_heel_lift: "Efficiency factor",
                    coupling_time: phaseAnalysis.transition.couplingTime,
                    effect: "Elastic energy utilization"
                },
                propulsion: {
                    contribution_to_heel_lift: heightAnalysis.FDI.heelLift,
                    contribution_to_air_height: "Via takeoff velocity",
                    duration: phaseAnalysis.propulsion.duration,
                    peak_power: phaseAnalysis.propulsion.peakPower
                },
                flight: {
                    contribution_to_air_height: heightAnalysis.FDI.airHeight,
                    energy_conversion: "Kinetic → Potential",
                    efficiency: this.calculateFlightEfficiency(heightAnalysis.FDI)
                }
            };
        }
        
        // Profilo di performance
        integration.performanceProfile = {
            dominant_quality: this.identifyDominantQuality(phaseAnalysis, heightAnalysis),
            limiting_factor: this.identifyLimitingFactor(phaseAnalysis, heightAnalysis),
            efficiency_rating: this.calculateOverallEfficiency(phaseAnalysis, heightAnalysis),
            athletic_profile: this.classifyAthleticProfile(phaseAnalysis, heightAnalysis)
        };
        
        return integration;
    }
    
    generateInsights(integratedAnalysis) {
        const insights = {
            strengths: [],
            weaknesses: [],
            recommendations: [],
            training_focus: [],
            technical_points: []
        };
        
        const phases = integratedAnalysis.phases;
        const heights = integratedAnalysis.heights;
        
        // Analisi punti di forza
        if (phases.propulsion.peakPower > 3000) {
            insights.strengths.push("Elevata capacità di generazione di potenza");
        }
        
        if (phases.transition.couplingTime < 0.08) {
            insights.strengths.push("Eccellente utilizzazione dell'energia elastica");
        }
        
        if (heights.FDI && heights.FDI.components?.heelLiftPercentage < 25) {
            insights.strengths.push("Proporzioni ottimali heel-lift/air-height");
        }
        
        // Analisi punti deboli
        if (phases.countermovement.duration > 0.4) {
            insights.weaknesses.push("Countermovement troppo lento");
            insights.recommendations.push("Allenamento velocità di movimento");
        }
        
        if (phases.propulsion.rateOfForceGeneration < 2000) {
            insights.weaknesses.push("Rate of Force Development limitato");
            insights.training_focus.push("Allenamento esplosivo e pliometrico");
        }
        
        if (heights.FDI && heights.FDI.components?.heelLiftPercentage > 30) {
            insights.weaknesses.push("Heel-lift eccessivo rispetto all'air-height");
            insights.technical_points.push("Ottimizzare timing e coordinazione articolare");
        }
        
        // Raccomandazioni specifiche
        const profile = integratedAnalysis.performanceProfile.athletic_profile;
        switch (profile) {
            case 'power_oriented':
                insights.training_focus.push("Mantenere forza, migliorare velocità");
                break;
            case 'speed_oriented':
                insights.training_focus.push("Mantenere velocità, migliorare forza");
                break;
            case 'elastic_oriented':
                insights.training_focus.push("Sfruttare capacità elastiche, aggiungere forza max");
                break;
            default:
                insights.training_focus.push("Allenamento bilanciato forza-velocità");
        }
        
        return insights;
    }
    
    generateReport(phases, heights, insights) {
        const report = {
            executive_summary: this.createExecutiveSummary(phases, heights, insights),
            detailed_metrics: this.createDetailedMetrics(phases, heights),
            biomechanical_analysis: this.createBiomechanicalAnalysis(phases, heights),
            recommendations: this.createRecommendations(insights),
            appendix: this.createTechnicalAppendix(phases, heights)
        };
        
        return report;
    }
    
    createExecutiveSummary(phases, heights, insights) {
        const recommended = heights.recommended;
        const totalHeight = recommended?.totalHeight || 0;
        
        return {
            jump_height: `${(totalHeight * 100).toFixed(1)} cm`,
            performance_level: this.classifyPerformanceLevel(totalHeight),
            primary_method: recommended?.method || 'N/A',
            key_strengths: insights.strengths.slice(0, 2),
            key_recommendations: insights.recommendations.slice(0, 2),
            overall_rating: this.calculateOverallRating(phases, heights, insights)
        };
    }
    
    classifyPerformanceLevel(height) {
        if (height > 0.55) return "Elite";
        if (height > 0.40) return "Advanced";
        if (height > 0.25) return "Intermediate";
        return "Novice";
    }
    
    calculateOverallRating(phases, heights, insights) {
        let score = 0;
        let maxScore = 0;
        
        // Scoring basato su metriche chiave
        const metrics = [
            { value: heights.recommended?.totalHeight || 0, max: 0.6, weight: 3 },
            { value: phases.propulsion?.peakPower || 0, max: 4000, weight: 2 },
            { value: 1 / (phases.transition?.couplingTime || 1), max: 12.5, weight: 2 },
            { value: phases.propulsion?.rateOfForceGeneration || 0, max: 5000, weight: 2 }
        ];
        
        metrics.forEach(metric => {
            score += Math.min(metric.value / metric.max, 1) * metric.weight;
            maxScore += metric.weight;
        });
        
        const normalizedScore = (score / maxScore) * 100;
        
        if (normalizedScore > 85) return "Excellent";
        if (normalizedScore > 70) return "Good";
        if (normalizedScore > 55) return "Average";
        return "Below Average";
    }
}

// Utilizzo del sistema completo
function demonstrateCompleteSystem() {
    console.log("=== SISTEMA COMPLETO DI ANALISI DEL SALTO ===\n");
    
    const analyzer = new JumpAnalyzer(1000);
    
    // Dati simulati
    const forceData = generateRealisticForceData(686, 600); // 70kg, 600ms
    const bodyWeight = 686; // N
    const anthropometrics = {
        footLength: 0.26,      // 26cm
        soleThickness: 0.02,   // 2cm
        ankleHeight: 0.08      // 8cm
    };
    
    // Analisi completa
    const analysis = analyzer.analyzeCompleteJump(forceData, bodyWeight, anthropometrics);
    
    // Output formattato
    console.log("RISULTATI EXECUTIVE SUMMARY:");
    console.log(`Altezza salto: ${analysis.report.executive_summary.jump_height}`);
    console.log(`Livello performance: ${analysis.report.executive_summary.performance_level}`);
    console.log(`Metodo raccomandato: ${analysis.report.executive_summary.primary_method}`);
    console.log(`Rating complessivo: ${analysis.report.executive_summary.overall_rating}`);
    
    console.log("\nCOMPARAZIONE METODI:");
    Object.entries(analysis.heights).forEach(([method, result]) => {
        if (result.totalHeight) {
            console.log(`${method}: ${(result.totalHeight*100).toFixed(1)}cm (${result.confidence})`);
        }
    });
    
    console.log("\nANALISI BIOMECCANICA:");
    console.log(`Countermovement: ${(analysis.phases.countermovement.depth*100).toFixed(1)}cm profondità`);
    console.log(`Transizione: ${(analysis.phases.transition.couplingTime*1000).toFixed(0)}ms coupling time`);
    console.log(`Propulsione: ${analysis.phases.propulsion.peakPower.toFixed(0)}W picco di potenza`);
    
    if (analysis.heights.FDI?.components) {
        console.log("\nCOMPONENTI ALTEZZA (FDI):");
        console.log(`Heel-lift: ${(analysis.heights.FDI.heelLift*100).toFixed(1)}cm (${analysis.heights.FDI.components.heelLiftPercentage.toFixed(1)}%)`);
        console.log(`Air height: ${(analysis.heights.FDI.airHeight*100).toFixed(1)}cm (${analysis.heights.FDI.components.airHeightPercentage.toFixed(1)}%)`);
    }
    
    console.log("\nRACCOMANDAZIONI:");
    analysis.insights.recommendations.forEach(rec => console.log(`• ${rec}`));
    
    return analysis;
}
```

### Integrazione con Sistemi di Monitoraggio

```javascript
// Sistema per tracking longitudinale
class JumpTracker {
    constructor() {
        this.sessions = [];
        this.analyzer = new JumpAnalyzer();
    }
    
    addSession(date, forceData, bodyWeight, anthropometrics, notes = '') {
        const analysis = this.analyzer.analyzeCompleteJump(forceData, bodyWeight, anthropometrics);
        
        const session = {
            date: date,
            analysis: analysis,
            notes: notes,
            key_metrics: this.extractKeyMetrics(analysis)
        };
        
        this.sessions.push(session);
        return this.analyzeProgress(session);
    }
    
    extractKeyMetrics(analysis) {
        return {
            jump_height: analysis.heights.recommended?.totalHeight || 0,
            peak_power: analysis.phases.propulsion?.peakPower || 0,
            coupling_time: analysis.phases.transition?.couplingTime || 0,
            rfd: analysis.phases.propulsion?.rateOfForceGeneration || 0,
            heel_lift_percentage: analysis.heights.FDI?.components?.heelLiftPercentage || 0
        };
    }
    
    analyzeProgress(currentSession) {
        if (this.sessions.length < 2) {
            return { status: 'baseline', message: 'Prima sessione registrata' };
        }
        
        const previous = this.sessions[this.sessions.length - 2];
        const current = currentSession;
        
        const improvements = [];
        const declines = [];
        
        Object.entries(current.key_metrics).forEach(([metric, value]) => {
            const previousValue = previous.key_metrics[metric];
            const change = ((value - previousValue) / previousValue) * 100;
            
            if (Math.abs(change) > 2) { // Soglia di significatività 2%
                if (change > 0) {
                    improvements.push(`${metric}: +${change.toFixed(1)}%`);
                } else {
                    declines.push(`${metric}: ${change.toFixed(1)}%`);
                }
            }
        });
        
        return {
            status: improvements.length > declines.length ? 'improving' : 'declining',
            improvements: improvements,
            declines: declines,
            trend_analysis: this.calculateTrends()
        };
    }
    
    generateProgressReport(timeframe = 30) { // Ultimi 30 giorni
        const cutoffDate = new Date(Date.now() - timeframe * 24 * 60 * 60 * 1000);
        const recentSessions = this.sessions.filter(s => new Date(s.date) > cutoffDate);
        
        if (recentSessions.length === 0) return null;
        
        return {
            timeframe: `${timeframe} giorni`,
            sessions_count: recentSessions.length,
            metrics_trends: this.analyzeMetricTrends(recentSessions),
            recommendations: this.generateProgressRecommendations(recentSessions)
        };
    }
}
```

Questo sistema completo integra perfettamente l'analisi biomeccanica delle fasi del salto con i tre metodi di calcolo dell'altezza, fornendo una piattaforma completa per valutazione, monitoraggio e ottimizzazione della performance nel salto verticale.Height = Math.pow(takeoffVelocity, 2) / (2 * g);  // Altezza durante volo
    const totalHeight = heelLift + airHeight;                  // Altezza totale
    
    return {
        heelLift: heelLift,
        airHeight: airHeight,
        totalHeight: totalHeight,
        heelLiftPercentage: (heelLift / totalHeight) * 100,
        airHeightPercentage: (airHeight / totalHeight) * 100,
        takeoffVelocity: takeoffVelocity
    };
}
```

### Perché il BDI Non Fornisce le Due Componenti Separate

Il metodo BDI calcola direttamente l'altezza totale del salto ma **non può separare** heel-lift e air height:

```javascript
// BDI fornisce solo:
const bdiResult = {
    totalHeight: 0.40,     // Altezza totale (heel-lift + air height)
    // ❌ Non può fornire heel-lift separatamente
    // ❌ Non può fornire air height separatamente
};

// Motivo: BDI analizza solo la fase di atterraggio
// Non ha informazioni sulla fase di spinta (heel-lift)
```

### Proporzioni Tipiche

Dalle ricerche biomeccaniche:
- **Heel-lift**: 20-25% dell'altezza totale (8-10 cm per salto di 40 cm)
- **Air height**: 75-80% dell'altezza totale (30-32 cm per salto di 40 cm)

### Esempio Numerico Completo

```javascript
function analyzeJumpExample_Complete() {
    // Dati esempio: atleta 70kg, salto di 40cm
    const bodyWeight = 686; // N
    const samplingRate = 1000; // Hz
    
    // Simula dati di forza per FDI
    const forceData = generateExampleForceData(bodyWeight, 400);
    
    // Analisi FDI (componenti separate)
    const fdiResult = calculateJumpComponents_FDI(forceData, bodyWeight, samplingRate);
    
    // Analisi BDI (solo altezza totale)
    const bdiResult = calculateBDI(forceData, bodyWeight, samplingRate);
    
    console.log("=== CONFRONTO FDI vs BDI ===");
    console.log(`FDI - Heel-lift: ${(fdiResult.heelLift*100).toFixed(1)} cm (${fdiResult.heelLiftPercentage.toFixed(1)}%)`);
    console.log(`FDI - Air height: ${(fdiResult.airHeight*100).toFixed(1)} cm (${fdiResult.airHeightPercentage.toFixed(1)}%)`);
    console.log(`FDI - Altezza totale: ${(fdiResult.totalHeight*100).toFixed(1)} cm`);
    console.log(`BDI - Altezza totale: ${(bdiResult.jumpHeight*100).toFixed(1)} cm`);
    console.log(`Differenza FDI-BDI: ${((fdiResult.totalHeight - bdiResult.jumpHeight)*1000).toFixed(1)} mm`);
    
    return { fdiResult, bdiResult };
}
``` (dai risultati del paper)

| Metodo | Bias medio | SD del bias | Limiti di accordo (95%) | R² | ICC |
|--------|------------|-------------|-------------------------|-----|-----|
| **FDI** | -0.4 mm | 0.9 mm | -2.2 a 1.5 mm | 0.989 | 0.994 |
| **BDI** | -0.1 mm | 1.2 mm | -2.3 a 2.2 mm | 0.983 | 0.995 |
| **FT+C** | -0.4 mm | 2.3 mm | -4.8 a 4.1 mm | 0.939 | 0.954 |

## Validazione e Controllo Qualità

```javascript
function validateJumpData(result, method) {
    const warnings = [];
    
    // Controlli generali
    if (result.jumpHeight < 0.05 || result.jumpHeight > 1.2) {
        warnings.push(`Altezza salto anomala: ${result.jumpHeight.toFixed(3)}m`);
    }
    
    // Controlli specifici per metodo
    switch(method) {
        case 'FDI':
            if (result.takeoffVelocity < 0.5 || result.takeoffVelocity > 5.0) {
                warnings.push(`Velocità decollo anomala: ${result.takeoffVelocity.toFixed(2)}m/s`);
            }
            break;
            
        case 'BDI':
            if (result.landingVelocity < 0.5 || result.landingVelocity > 5.0) {
                warnings.push(`Velocità atterraggio anomala: ${result.landingVelocity.toFixed(2)}m/s`);
            }
            break;
            
        case 'FTC':
            if (result.flightTime < 0.15 || result.flightTime > 1.0) {
                warnings.push(`Tempo di volo anomalo: ${result.flightTime.toFixed(3)}s`);
            }
            break;
    }
    
    return {
        isValid: warnings.length === 0,
        warnings: warnings,
        confidence: getConfidenceLevel(method, result)
    };
}

function getConfidenceLevel(method, result) {
    // Basato sui dati di variabilità dal paper
    const confidenceLevels = {
        'FDI': 'Alta (±1mm)',
        'BDI': 'Alta (±2-3mm)', 
        'FTC': 'Media (±11mm)'
    };
    return confidenceLevels[method] || 'Sconosciuta';
}
```

## Raccomandazioni Pratiche

### Quando usare ciascun metodo:

1. **FDI**: Gold standard per countermovement jump e squat jump
2. **BDI**: Quando FDI non è applicabile (drop jump) o dati di push-off compromessi
3. **FT+C**: Applicazioni sul campo quando non è disponibile una pedana di forza

### Considerazioni tecniche:

- **Frequenza di campionamento**: ≥1000 Hz per FDI/BDI, ≥200 Hz per FT+C
- **Filtro dati**: Passa-basso 10-50 Hz per ridurre rumore
- **Calibrazione**: Verificare sempre la calibrazione prima delle misurazioni
- **Quiet standing**: Minimo 2-3 secondi prima e dopo il salto

Questo framework fornisce le basi matematiche discrete necessarie per implementare accuratamente tutti e tre i metodi nel tuo sistema di analisi del salto.

## Sitografia

- https://www.tandfonline.com/doi/epdf/10.1080/02640414.2022.2059319

>[Torna all'indice](readme.md#fasi-progetto)
