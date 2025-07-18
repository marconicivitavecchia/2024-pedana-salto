>[Torna a metodi di calcolo salti](calcstrategies.md)

# Parte II: Implementazione Pratica e Esempi Software

## Implementazioni JavaScript

### 1. Forward Double Integration (FDI)

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
    
    // Trova l'indice di decollo
    const takeoffIndex = netForce.findIndex((force, i) => 
        i > 0 && force < -bodyWeight
    );
    
    // Calcola componenti altezza
    const heelLift = displacement[takeoffIndex];
    const takeoffVelocity = velocity[takeoffIndex];
    const airHeight = Math.pow(takeoffVelocity, 2) / (2 * g);
    const jumpHeight = heelLift + airHeight;
    
    return {
        jumpHeight: jumpHeight,
        heelLift: heelLift,
        airHeight: airHeight,
        takeoffVelocity: takeoffVelocity,
        heelLiftPercentage: (heelLift / jumpHeight) * 100,
        airHeightPercentage: (airHeight / jumpHeight) * 100
    };
}
```

### 2. Backward Double Integration (BDI)

```javascript
function calculateBDI(forceData, bodyWeight, samplingRate) {
    const g = 9.81;
    const dt = 1 / samplingRate;
    const mass = bodyWeight / g;
    
    // Identifica la fase di atterraggio
    const landingStart = findLandingStart(forceData, bodyWeight);
    const landingEnd = findLandingEnd(forceData, bodyWeight, landingStart);
    
    // Estrai i dati di atterraggio
    const landingForces = forceData.slice(landingStart, landingEnd);
    
    // INVERSIONE TEMPORALE - Punto chiave!
    const reversedForces = [...landingForces].reverse();
    
    // Calcola accelerazione sui dati invertiti
    const acceleration = reversedForces.map(force => (force - bodyWeight) / mass);
    
    // Doppia integrazione normale sui dati invertiti
    let velocity = [0];      // Condizione iniziale nota: quiet standing
    let displacement = [0];  // Condizione iniziale nota: posizione riferimento
    
    for (let i = 1; i < acceleration.length; i++) {
        velocity[i] = velocity[i-1] + acceleration[i] * dt;
        displacement[i] = displacement[i-1] + velocity[i] * dt;
    }
    
    // Estrai risultati
    const jumpHeight = Math.max(...displacement);
    const reconstructedTakeoffVelocity = Math.abs(velocity[velocity.length - 1]);
    
    return {
        jumpHeight: jumpHeight,
        reconstructedTakeoffVelocity: reconstructedTakeoffVelocity,
        heelLift: null, // Non separabile con BDI
        airHeight: null // Non separabile con BDI
    };
}

function findLandingStart(forceData, bodyWeight) {
    return forceData.findIndex(force => force > 2 * bodyWeight);
}

function findLandingEnd(forceData, bodyWeight, startIndex) {
    const tolerance = bodyWeight * 0.1;
    
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

### 3. Flight Time + Constant (FT+C)

```javascript
function calculateFTC(flightTime, footLength, soleThickness, ankleHeight) {
    const g = 9.81;
    
    // Calcolo altezza base dal tempo di volo
    const baseHeight = (g * Math.pow(flightTime, 2)) / 8;
    
    // Calcolo costante antropometrica (Wade et al., 2020)
    const heelLiftConstant = (0.88 * footLength) + soleThickness - ankleHeight;
    
    // Altezza totale del salto
    const jumpHeight = baseHeight + heelLiftConstant;
    
    return {
        jumpHeight: jumpHeight,
        baseHeight: baseHeight,
        heelLiftConstant: heelLiftConstant,
        flightTime: flightTime,
        heelLiftPercentage: (heelLiftConstant / jumpHeight) * 100,
        airHeightPercentage: (baseHeight / jumpHeight) * 100
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

## Sistema di Validazione e Confronto

```javascript
function validateAllMethods(forceData, bodyWeight, samplingRate, anthropometrics) {
    const results = {};
    const warnings = [];
    
    // Calcola con tutti i metodi disponibili
    try {
        results.fdi = calculateFDI(forceData, bodyWeight, samplingRate);
        
        results.bdi = calculateBDI(forceData, bodyWeight, samplingRate);
        
        const flightTime = detectFlightTime(forceData, bodyWeight, samplingRate);
        if (flightTime && anthropometrics) {
            results.ftc = calculateFTC(
                flightTime, 
                anthropometrics.footLength,
                anthropometrics.soleThickness, 
                anthropometrics.ankleHeight
            );
        }
        
        // Confronta i risultati
        const validation = performCrossValidation(results);
        
        return {
            results: results,
            validation: validation,
            recommendedMethod: selectRecommendedMethod(results, validation)
        };
        
    } catch (error) {
        return {
            error: error.message,
            results: results,
            validation: { isValid: false }
        };
    }
}

function performCrossValidation(results) {
    const warnings = [];
    
    if (results.fdi && results.bdi) {
        const heightDiff = Math.abs(results.fdi.jumpHeight - results.bdi.jumpHeight);
        if (heightDiff > 0.02) { // >2cm
            warnings.push(`Differenza significativa FDI-BDI: ${(heightDiff*100).toFixed(1)}cm`);
        }
    }
    
    if (results.fdi && results.ftc) {
        const heightDiff = Math.abs(results.fdi.jumpHeight - results.ftc.jumpHeight);
        if (heightDiff > 0.05) { // >5cm
            warnings.push(`Differenza elevata FDI-FT+C: ${(heightDiff*100).toFixed(1)}cm`);
        }
    }
    
    // Controllo consistenza fisica
    if (results.fdi) {
        const expectedAirHeight = Math.pow(results.fdi.takeoffVelocity, 2) / (2 * 9.81);
        const airHeightError = Math.abs(expectedAirHeight - results.fdi.airHeight) / results.fdi.airHeight;
        
        if (airHeightError > 0.01) {
            warnings.push(`Errore conservazione energia: ${(airHeightError*100).toFixed(2)}%`);
        }
    }
    
    return {
        isValid: warnings.length === 0,
        warnings: warnings,
        confidence: warnings.length === 0 ? "Alta" : "Media"
    };
}
```

## Analisi Completa delle Fasi del Salto

```javascript
function performCompleteJumpAnalysis(forceData, bodyWeight, samplingRate) {
    // Calcola velocità e spostamento tramite FDI
    const fdiResult = calculateFDI(forceData, bodyWeight, samplingRate);
    
    // Identifica le fasi
    const phases = identifyJumpPhases(forceData, fdiResult.velocity, bodyWeight);
    
    // Analizza ogni fase
    const analysis = {
        quietStanding: analyzeQuietStanding(
            forceData.slice(0, phases.quietEnd)
        ),
        
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
        performanceProfile: generatePerformanceProfile(analysis)
    };
    
    return analysis;
}

function identifyJumpPhases(forceData, velocity, bodyWeight) {
    // Identifica i punti di transizione tra le fasi
    const quietEnd = forceData.findIndex((force, i) => 
        i > 100 && Math.abs(force - bodyWeight) > bodyWeight * 0.05
    );
    
    const velocityZero = velocity.findIndex(v => v <= 0);
    const cmEnd = velocityZero > 0 ? velocityZero : quietEnd + 200;
    
    const takeoffIndex = forceData.findIndex((force, i) => 
        i > cmEnd && force < bodyWeight * 0.1
    );
    
    // Stima tempo di volo
    const landingIndex = forceData.findIndex((force, i) => 
        i > takeoffIndex && force > bodyWeight * 2
    );
    
    const flightTime = landingIndex > takeoffIndex ? 
        (landingIndex - takeoffIndex) / 1000 : 0; // Assumendo 1000 Hz
    
    return {
        quietEnd: quietEnd,
        cmStart: quietEnd,
        cmEnd: cmEnd,
        transStart: cmEnd - 50,
        transEnd: cmEnd + 50,
        propStart: cmEnd,
        propEnd: takeoffIndex,
        flightTime: flightTime
    };
}

function analyzeCountermovement(forceData, velocity, displacement, bodyWeight) {
    return {
        duration: forceData.length / 1000, // Assumendo 1000 Hz
        depth: Math.abs(Math.min(...displacement)),
        minForce: Math.min(...forceData),
        maxVelocityDown: Math.min(...velocity),
        forceReduction: bodyWeight - Math.min(...forceData)
    };
}

function analyzeTransition(forceData, velocity, samplingRate) {
    const transitionPoint = velocity.findIndex(v => v >= 0);
    
    return {
        couplingTime: forceData.length / samplingRate * 1000, // ms
        maxAcceleration: Math.max(...forceData.map((f, i) => 
            i > 0 ? (f - forceData[i-1]) * samplingRate : 0
        )),
        forceAtTransition: transitionPoint >= 0 ? forceData[transitionPoint] : 0
    };
}

function analyzePropulsion(forceData, velocity, displacement, bodyWeight, samplingRate) {
    return {
        duration: forceData.length / samplingRate,
        peakForce: Math.max(...forceData),
        averageForce: forceData.reduce((sum, f) => sum + f, 0) / forceData.length,
        takeoffVelocity: velocity[velocity.length - 1],
        heelLift: displacement[displacement.length - 1],
        peakPower: calculatePeakPower(forceData, velocity),
        rateOfForceGeneration: calculateRFD(forceData, samplingRate, bodyWeight)
    };
}

function calculatePeakPower(forceData, velocity) {
    let maxPower = 0;
    for (let i = 0; i < forceData.length && i < velocity.length; i++) {
        const power = forceData[i] * Math.max(0, velocity[i]);
        if (power > maxPower) maxPower = power;
    }
    return maxPower;
}

function calculateRFD(forceData, samplingRate, bodyWeight) {
    // Rate of Force Development nei primi 100ms della spinta
    const rfdWindow = Math.floor(0.1 * samplingRate); // 100ms
    const startIndex = forceData.findIndex(f => f > bodyWeight);
    
    if (startIndex + rfdWindow >= forceData.length) return 0;
    
    const forceChange = forceData[startIndex + rfdWindow] - forceData[startIndex];
    const timeChange = rfdWindow / samplingRate;
    
    return forceChange / timeChange; // N/s
}

function analyzeFlightPhase(flightTime, takeoffVelocity, landingVelocity) {
    const g = 9.81;
    
    return {
        flightTime: flightTime,
        airHeight: Math.pow(takeoffVelocity, 2) / (2 * g),
        timeToApex: takeoffVelocity / g,
        flightSymmetry: Math.abs(takeoffVelocity - Math.abs(landingVelocity || takeoffVelocity)) / takeoffVelocity
    };
}
```

## Filtraggio dei Segnali

### Per Campionamento a 30 kSPS

```javascript
class SignalProcessor {
    constructor(originalFs = 30000) {
        this.originalFs = originalFs;
        this.targetFs = 2000; // Frequenza target dopo decimazione
        this.decimationFactor = Math.floor(originalFs / this.targetFs);
    }
    
    processForJumpAnalysis(rawForceData, method = 'FDI') {
        // Step 1: Decimazione con anti-aliasing
        const decimated = this.decimateSignal(rawForceData);
        
        // Step 2: Filtro specifico per metodo
        const filtered = this.applyMethodSpecificFilter(decimated.data, method);
        
        // Step 3: Rimozione drift
        const driftCorrected = this.removeDrift(filtered);

>[Torna a metodi di calcolo salti](calcstrategies.md)
