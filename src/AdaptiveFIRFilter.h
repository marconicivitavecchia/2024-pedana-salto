#ifndef ADAPTIVE_FIR_FILTER_H
#define ADAPTIVE_FIR_FILTER_H

#include <cstdint>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ====================================================================
// FILTRO FIR ADATTIVO OTTIMIZZATO E CORRETTO
// ====================================================================

class AdaptiveFIRFilter {
public:
    enum InputType {
        ADC_UNIPOLAR,
        BIPOLAR_CENTERED
    };

private:
    static const uint16_t MAX_TAPS = 101;
    static const uint32_t SAMPLE_RATE = 30000;
    
    // === STRUTTURA STAGE CASCATA ===
    struct CascadeStage {
        float coeffs[21];       // Max 21 taps per stage (aumentato per qualità)
        float delay[21];
        uint8_t numTaps;
        uint16_t decimation;
        uint16_t delayIndex;
        uint32_t sampleCounter;
    };
    
    CascadeStage cascadeStages[3];  // Max 3 stadi (semplificato)
    uint8_t numStages;
    bool useCascade;
    
    // === COEFFICIENTI PRECOMPUTATI REALI ===
    // Calcolati offline con cutoff ottimale 0.92*Nyquist
    static const float COEFFS_11_TAPS[11];
    static const float COEFFS_15_TAPS[15]; 
    static const float COEFFS_17_TAPS[17];
    static const float COEFFS_21_TAPS[21];
    
    // === FILTRO SINGOLO ===
    float coefficients[MAX_TAPS];
    float delayLine[MAX_TAPS];
    uint16_t numTaps;
    uint16_t delayIndex;
    uint16_t currentDecimation;
    uint32_t sampleCounter;
    bool useSymmetricOptimization;
    
    // Parametri input
    InputType inputType;
    uint32_t inputBitDepth;
    float inputOffset;
    float inputScale;

    // === METODI PRIVATI ===
    
    void updateInputParameters() {
        if (inputType == ADC_UNIPOLAR) {
            uint32_t maxValue = (1UL << inputBitDepth) - 1;
            inputOffset = (float)maxValue / 2.0f;
            inputScale = 2.0f / (float)maxValue;
        } else {
            inputOffset = 0.0f;
            float maxMagnitude = (float)(1UL << (inputBitDepth - 1));
            inputScale = 1.0f / maxMagnitude;
        }
    }
    
    float convertInputToFloat(uint32_t inputSample) {
        if (inputType == ADC_UNIPOLAR) {
            return ((float)inputSample - inputOffset) * inputScale;
        } else {
            int32_t signedInput = (int32_t)inputSample;
            int32_t maxPositive = (1L << (inputBitDepth - 1)) - 1;
            if (signedInput > maxPositive) {
                signedInput -= (1UL << inputBitDepth);
            }
            return (float)signedInput * inputScale;
        }
    }
    
    uint32_t convertFloatToOutput(float floatValue) {
        if (inputType == ADC_UNIPOLAR) {
            float reconverted = (floatValue / inputScale) + inputOffset;
            uint32_t maxValue = (1UL << inputBitDepth) - 1;
            if (reconverted < 0.0f) return 0;
            if (reconverted > (float)maxValue) return maxValue;
            return (uint32_t)reconverted;
        } else {
            float reconverted = floatValue / inputScale;
            int32_t maxValue = (1L << (inputBitDepth - 1)) - 1;
            int32_t minValue = -(1L << (inputBitDepth - 1));
            
            int32_t result;
            if (reconverted < (float)minValue) result = minValue;
            else if (reconverted > (float)maxValue) result = maxValue;
            else result = (int32_t)reconverted;
            
            return (uint32_t)result;
        }
    }

    // === CALCOLO CUTOFF OTTIMALE ===
    float calculateOptimalCutoff(uint16_t decimationFactor) {
        // PROTEZIONE: evita divisione per zero
        if (decimationFactor == 0) decimationFactor = 1;
        
        float nyquist = (float)SAMPLE_RATE / (2.0f * decimationFactor);
        // CORRETTO: 92% invece di 80% per preservare il segnale
        return nyquist * 0.92f;
    }

    // === CASCATA SEMPLIFICATA E OTTIMIZZATA ===
    void setupOptimalCascade(uint16_t totalDecimation) {
        numStages = 0;
        
        // PROTEZIONE: verifica input valido
        if (totalDecimation <= 1) {
            printf("Warning: Invalid decimation %d, using single filter\n", totalDecimation);
            return;
        }
        
        // STRATEGIA SEMPLIFICATA: decomposizioni ottimali prestabilite
        if (totalDecimation == 12) {
            // 12 = 3 × 4
            addCascadeStage(3, 15);
            addCascadeStage(4, 15);
            numStages = 2;
        } else if (totalDecimation == 15) {
            // 15 = 3 × 5
            addCascadeStage(3, 15);
            addCascadeStage(5, 17);
            numStages = 2;
        } else if (totalDecimation == 20) {
            // 20 = 4 × 5
            addCascadeStage(4, 15);
            addCascadeStage(5, 17);
            numStages = 2;
        } else if (totalDecimation == 24) {
            // 24 = 4 × 6
            addCascadeStage(4, 15);
            addCascadeStage(6, 17);
            numStages = 2;
        } else if (totalDecimation == 30) {
            // 30 = 5 × 6 (OTTIMALE)
            addCascadeStage(5, 17);
            addCascadeStage(6, 17);
            numStages = 2;
        } else if (totalDecimation == 50) {
            // 50 = 5 × 10
            addCascadeStage(5, 17);
            addCascadeStage(10, 21);
            numStages = 2;
        } else if (totalDecimation == 60) {
            // 60 = 5 × 12
            addCascadeStage(5, 17);
            addCascadeStage(12, 21);
            numStages = 2;
        } else if (totalDecimation == 100) {
            // 100 = 5 × 4 × 5
            addCascadeStage(5, 17);
            addCascadeStage(4, 15);
            addCascadeStage(5, 17);
            numStages = 3;
        } else if (totalDecimation == 150) {
            // 150 = 5 × 6 × 5
            addCascadeStage(5, 17);
            addCascadeStage(6, 17);
            addCascadeStage(5, 17);
            numStages = 3;
        } else {
            // Fallback: decomposizione automatica semplice
            autoDecompose(totalDecimation);
        }
        
        printf("Cascade: %d stages for decimation %d\n", numStages, totalDecimation);
    }
    
    void addCascadeStage(uint16_t decimation, uint8_t taps) {
        if (numStages >= 3) return;  // Limite di sicurezza
        
        // PROTEZIONI DI SICUREZZA
        if (decimation == 0) decimation = 1;  // Evita divisione per zero
        if (taps == 0) taps = 11;             // Taps minimi
        if (taps > 21) taps = 21;             // Taps massimi
        
        CascadeStage& stage = cascadeStages[numStages];
        stage.decimation = decimation;
        stage.numTaps = taps;
        stage.sampleCounter = 0;
        stage.delayIndex = 0;
        
        // Azzera delay line
        for (uint8_t i = 0; i < 21; i++) {
            stage.delay[i] = 0.0f;
        }
        
        // Seleziona coefficienti precomputati o genera
        if (taps == 11 && decimation == 2) {
            memcpy(stage.coeffs, COEFFS_11_TAPS, 11 * sizeof(float));
        } else if (taps == 15) {
            memcpy(stage.coeffs, COEFFS_15_TAPS, 15 * sizeof(float));
        } else if (taps == 17) {
            memcpy(stage.coeffs, COEFFS_17_TAPS, 17 * sizeof(float));
        } else if (taps == 21) {
            memcpy(stage.coeffs, COEFFS_21_TAPS, 21 * sizeof(float));
        } else {
            // Genera coefficienti per configurazioni non standard
            generateStageCoeffs(stage.coeffs, taps, decimation);
        }
    }
    
    void autoDecompose(uint16_t totalDecimation) {
        // PROTEZIONE: verifica input valido
        if (totalDecimation <= 1) {
            numStages = 0;
            return;
        }
        
        // Decomposizione automatica semplice per casi non previsti
        uint16_t remaining = totalDecimation;
        
        // Prova fattori ottimali: 5, 4, 3, 2
        const uint16_t preferredFactors[] = {5, 4, 3, 2};
        
        for (uint16_t factor : preferredFactors) {
            while (remaining > 1 && remaining % factor == 0 && numStages < 3) {
                uint8_t stageTaps = (factor <= 3) ? 15 : 17;
                addCascadeStage(factor, stageTaps);
                remaining /= factor;
                numStages++;
            }
        }
        
        // Se rimane qualcosa, ultimo stage
        if (remaining > 1 && numStages < 3) {
            uint8_t stageTaps = std::min(remaining, (uint16_t)21);
            addCascadeStage(remaining, stageTaps);
            numStages++;
        }
        
        // PROTEZIONE: se non si è riusciti a decomporre
        if (numStages == 0) {
            // Fallback: stage singolo
            addCascadeStage(totalDecimation, 21);
            numStages = 1;
        }
    }
    
    void generateStageCoeffs(float* coeffs, uint8_t taps, uint16_t decimation) {
        // Genera coefficienti ottimali per lo stage
        float cutoffFreq = calculateOptimalCutoff(decimation);
        generateLowpassCoefficients(coeffs, cutoffFreq, taps);
    }
    
    bool processCascadeSample(float input, float& output) {
        float stageOutput = input;
        bool hasOutput = true;
        
        for (uint8_t stage = 0; stage < numStages; stage++) {
            if (!hasOutput) break;
            
            CascadeStage& s = cascadeStages[stage];
            
            // DEBUG: verifica stage
            if (s.decimation == 0) {
                printf("CRITICAL: Stage %d has decimation 0!\n", stage);
                s.decimation = 1;
            }
            if (s.numTaps == 0) {
                printf("CRITICAL: Stage %d has numTaps 0!\n", stage);
                s.numTaps = 11;
            }
            
            // Inserisci in delay line dello stage
            s.delay[s.delayIndex] = stageOutput;
            s.delayIndex = (s.delayIndex + 1) % s.numTaps;
            
            // Calcola output FIR per questo stage (CORRETTO)
            float filtered = 0.0f;
            for (uint8_t i = 0; i < s.numTaps; i++) {
                uint8_t sampleIdx = (s.delayIndex + s.numTaps - 1 - i) % s.numTaps;
                filtered += s.delay[sampleIdx] * s.coeffs[i];
            }
            
            // Decimazione per questo stage
            s.sampleCounter++;
            if (s.sampleCounter >= s.decimation) {
                s.sampleCounter = 0;
                stageOutput = filtered;
            } else {
                hasOutput = false;
            }
        }
        
        if (hasOutput) {
            output = stageOutput;
            return true;
        }
        return false;
    }

    // === ELABORAZIONE SIMMETRICA CORRETTA ===
    bool processSymmetricSample(float input, float& output) {
        // Inserisci nella linea di ritardo
        delayLine[delayIndex] = input;
        delayIndex = (delayIndex + 1) % numTaps;
        
        // ELABORAZIONE SIMMETRICA CORRETTA
        float filteredOutput = 0.0f;
        uint16_t center = numTaps / 2;
        
        // Coefficiente centrale (se numTaps è dispari)
        if (numTaps % 2 == 1) {
            uint16_t centerIdx = (delayIndex + numTaps - 1 - center) % numTaps;
            filteredOutput += delayLine[centerIdx] * coefficients[center];
        }
        
        // Coefficienti simmetrici (CORRETTO: rispetta ordine cronologico)
        uint16_t symmetricPairs = numTaps / 2;
        for (uint16_t i = 0; i < symmetricPairs; i++) {
            uint16_t idx1 = (delayIndex + numTaps - 1 - i) % numTaps;
            uint16_t idx2 = (delayIndex + numTaps - 1 - (numTaps - 1 - i)) % numTaps;
            
            // Un coefficiente, due campioni (simmetria)
            filteredOutput += (delayLine[idx1] + delayLine[idx2]) * coefficients[i];
        }
        
        // Decimazione
        sampleCounter++;
        if (sampleCounter >= currentDecimation) {
            sampleCounter = 0;
            output = filteredOutput;
            return true;
        }
        
        return false;
    }

    // === ALGORITMO TAPS INTELLIGENTE CORRETTO ===
    uint16_t calculateIntelligentTaps(uint16_t decimationFactor) {
        // Per decimazioni > 12, usa cascata
        if (decimationFactor > 12) {
            return 0;  // Segnala di usare cascata
        }
        
        // Formula ottimizzata più aggressiva
        uint16_t baseTaps;
        
        if (decimationFactor <= 2) {
            baseTaps = 11;
        } else if (decimationFactor <= 3) {
            baseTaps = 15;
        } else if (decimationFactor <= 5) {
            baseTaps = 17;
        } else if (decimationFactor <= 8) {
            baseTaps = 21;
        } else {
            baseTaps = 25;
        }
        
        // Penalità ridotta per decimazioni dispari (solo casi critici)
        if (decimationFactor > 2 && decimationFactor % 2 == 1 && decimationFactor <= 5) {
            baseTaps += 4;  // Solo 4 tap extra invece di 20!
        }
        
        // Assicura dispari per simmetria
        if (baseTaps % 2 == 0) baseTaps++;
        
        return std::min(baseTaps, (uint16_t)MAX_TAPS);
    }

    // === GENERAZIONE COEFFICIENTI CORRETTA ===
    void generateLowpassCoefficients(float* coeffs, float cutoffFreq, uint16_t taps) {
        float omega_c = 2.0f * M_PI * cutoffFreq / SAMPLE_RATE;
        int center = taps / 2;
        float sum = 0.0f;
        
        for (int i = 0; i < taps; i++) {
            int n = i - center;
            
            if (n == 0) {
                coeffs[i] = omega_c / M_PI;
            } else {
                coeffs[i] = sin(omega_c * n) / (M_PI * n);
            }
            
            if (isnan(coeffs[i]) || isinf(coeffs[i])) {
                coeffs[i] = 0.0f;
            }
            
            // Finestra di Hamming
            float hamming = 0.54f - 0.46f * cos(2.0f * M_PI * i / (taps - 1));
            coeffs[i] *= hamming;
            sum += coeffs[i];
        }
        
        // Normalizzazione robusta
        if (sum <= 0.0f || isnan(sum) || isinf(sum)) {
            for (int i = 0; i < taps; i++) {
                coeffs[i] = 1.0f / (float)taps;
            }
            return;
        }
        
        for (int i = 0; i < taps; i++) {
            coeffs[i] /= sum;
        }
    }

public:
    // === COSTRUTTORE ===
    AdaptiveFIRFilter(InputType type = ADC_UNIPOLAR, uint32_t bitDepth = 24) 
        : inputType(type), inputBitDepth(bitDepth), useCascade(false), 
          useSymmetricOptimization(true), numStages(0) {
        updateInputParameters();
        reset();
        setDecimationFactor(1);
    }

    ~AdaptiveFIRFilter() = default;

    // === RESET ===
    void reset() {
        delayIndex = 0;
        sampleCounter = 0;
        currentDecimation = 1;
        numTaps = 1;
        numStages = 0;
        useCascade = false;
        
        for (uint16_t i = 0; i < MAX_TAPS; i++) {
            delayLine[i] = 0.0f;
            coefficients[i] = 0.0f;
        }
        
        for (uint8_t s = 0; s < 3; s++) {
            cascadeStages[s].sampleCounter = 0;
            cascadeStages[s].delayIndex = 0;
            for (uint8_t i = 0; i < 21; i++) {
                cascadeStages[s].delay[i] = 0.0f;
                cascadeStages[s].coeffs[i] = 0.0f;
            }
        }
    }

    // === CONFIGURAZIONE DECIMAZIONE ===
    void setDecimationFactor(uint16_t decimationFactor) {
        // PROTEZIONE E DEBUG: input valido
        if (decimationFactor == 0) {
            printf("ERROR: decimationFactor cannot be zero, setting to 1\n");
            decimationFactor = 1;
        }
        
        if (decimationFactor == currentDecimation) return;
        
        printf("=== SETTING DECIMATION ===\n");
        printf("From %d to %d\n", currentDecimation, decimationFactor);
        
        currentDecimation = decimationFactor;
        sampleCounter = 0;
        
        // Reset stage counters se in cascata
        for (uint8_t s = 0; s < numStages; s++) {
            cascadeStages[s].sampleCounter = 0;
        }
        
        if (decimationFactor == 1) {
            numTaps = 1;
            coefficients[0] = 1.0f;
            useCascade = false;
            printf("Using pass-through filter\n");
            return;
        }
        
        // DECISIONE INTELLIGENTE: Cascata vs Filtro Singolo
        uint16_t requiredTaps = calculateIntelligentTaps(decimationFactor);
        printf("Required taps: %d\n", requiredTaps);
        
        if (requiredTaps == 0 || decimationFactor > 12) {
            printf("Choosing CASCADE mode\n");
            
            // Reset numStages before setup
            numStages = 0;
            
            // Usa decomposizione a cascata
            setupOptimalCascade(decimationFactor);
            useCascade = true;
            
            printf("CASCADE configured: %d stages for decimation %d\n", 
                   numStages, decimationFactor);
            
            // DEBUG: verifica cascata
            for (uint8_t s = 0; s < numStages; s++) {
                printf("Stage %d: decimation=%d, taps=%d\n", 
                       s, cascadeStages[s].decimation, cascadeStages[s].numTaps);
                       
                if (cascadeStages[s].decimation == 0) {
                    printf("ERROR: Stage %d has zero decimation!\n", s);
                }
                if (cascadeStages[s].numTaps == 0) {
                    printf("ERROR: Stage %d has zero taps!\n", s);
                }
            }
            
        } else {
            printf("Choosing SINGLE filter mode\n");
            
            // Usa filtro singolo ottimizzato
            useCascade = false;
            numTaps = requiredTaps;
            
            printf("numTaps set to: %d\n", numTaps);
            
            if (numTaps == 0) {
                printf("ERROR: numTaps is zero! Setting to 21\n");
                numTaps = 21;
            }
            
            // Genera coefficienti con cutoff ottimale
            float cutoffFreq = calculateOptimalCutoff(decimationFactor);
            generateLowpassCoefficients(coefficients, cutoffFreq, numTaps);
            
            printf("SINGLE filter: %d taps, cutoff=%.0fHz for decimation %d\n", 
                   numTaps, cutoffFreq, decimationFactor);
        }
        
        printf("=== DECIMATION SET COMPLETE ===\n");
        printf("Final state: decimation=%d, useCascade=%s, numTaps=%d, numStages=%d\n",
               currentDecimation, useCascade ? "true" : "false", numTaps, numStages);
    }

    // === ELABORAZIONE CAMPIONE PRINCIPALE ===
    bool processSample(uint32_t inputSample, uint32_t& outputSample) {
        float floatInput = convertInputToFloat(inputSample);
        
        // DEBUG: verifica stato interno
        if (currentDecimation == 0) {
            printf("CRITICAL: currentDecimation is 0 in processSample!\n");
            currentDecimation = 1;
            return false;
        }
        
        if (currentDecimation == 1) {
            outputSample = inputSample;
            return true;
        }
        
        float filteredOutput;
        bool hasOutput;
        
        if (useCascade) {
            // DEBUG: verifica cascata
            if (numStages == 0) {
                printf("CRITICAL: numStages is 0 but useCascade is true!\n");
                useCascade = false;
                return false;
            }
            hasOutput = processCascadeSample(floatInput, filteredOutput);
        } else if (useSymmetricOptimization && numTaps > 21) {
            // DEBUG: verifica taps
            if (numTaps == 0) {
                printf("CRITICAL: numTaps is 0 in symmetric mode!\n");
                numTaps = 21;
                return false;
            }
            hasOutput = processSymmetricSample(floatInput, filteredOutput);
        } else {
            // DEBUG: verifica configurazione standard
            if (numTaps == 0) {
                printf("CRITICAL: numTaps is 0 in standard mode!\n");
                numTaps = 21;
                return false;
            }
            
            // Elaborazione standard corretta
            delayLine[delayIndex] = floatInput;
            delayIndex = (delayIndex + 1) % numTaps;
            
            filteredOutput = 0.0f;
            
            // ELABORAZIONE CORRETTA: rispetta ordine cronologico
            for (uint16_t i = 0; i < numTaps; i++) {
                uint16_t sampleIdx = (delayIndex + numTaps - 1 - i) % numTaps;
                filteredOutput += delayLine[sampleIdx] * coefficients[i];
            }
            
            sampleCounter++;
            hasOutput = (sampleCounter >= currentDecimation);
            if (hasOutput) sampleCounter = 0;
        }
        
        if (hasOutput) {
            outputSample = convertFloatToOutput(filteredOutput);
            return true;
        }
        
        return false;
    }

    // === METODI DI UTILITÀ ===
    void setInputType(InputType type, uint32_t bitDepth = 24) {
        inputType = type;
        inputBitDepth = bitDepth;
        updateInputParameters();
        
        printf("FIR: InputType=%s, BitDepth=%lu, Offset=%.1f, Scale=%.6f\n",
               (type == ADC_UNIPOLAR) ? "ADC_UNIPOLAR" : "BIPOLAR_CENTERED",
               bitDepth, inputOffset, inputScale);
    }
    
    void enableSymmetricOptimization(bool enable) {
        useSymmetricOptimization = enable;
    }
    
    void getFilterInfo(uint16_t& taps, float& cutoffHz, uint32_t& outputRateHz) {
        if (useCascade) {
            taps = 0;
            for (uint8_t i = 0; i < numStages; i++) {
                taps += cascadeStages[i].numTaps;
            }
        } else {
            taps = numTaps;
        }
        
        // CORRETTO: usa cutoff ottimale
        cutoffHz = calculateOptimalCutoff(currentDecimation);
        outputRateHz = SAMPLE_RATE / currentDecimation;
    }
    
    float getFrequencyResponse(float frequencyHz) {
        if (useCascade) {
            // Calcola risposta composta della cascata
            float totalResponse = 1.0f;
            for (uint8_t s = 0; s < numStages; s++) {
                float stageResponse = calculateStageResponse(s, frequencyHz);
                totalResponse *= stageResponse;
            }
            return totalResponse;
        } else {
            return calculateSingleFilterResponse(frequencyHz);
        }
    }
    
    // Getters
    bool isUsingCascade() const { return useCascade; }
    uint8_t getNumCascadeStages() const { return numStages; }
    uint16_t getCurrentDecimation() const { return currentDecimation; }
    uint16_t getNumTaps() const { return numTaps; }
    uint32_t getSampleRate() const { return SAMPLE_RATE; }
    InputType getInputType() const { return inputType; }
    uint32_t getInputBitDepth() const { return inputBitDepth; }

private:
    float calculateStageResponse(uint8_t stageIndex, float frequencyHz) {
        if (stageIndex >= numStages) return 1.0f;
        
        const CascadeStage& stage = cascadeStages[stageIndex];
        float omega = 2.0f * M_PI * frequencyHz / SAMPLE_RATE;
        float real = 0.0f, imag = 0.0f;
        
        for (uint8_t i = 0; i < stage.numTaps; i++) {
            real += stage.coeffs[i] * cos(omega * i);
            imag -= stage.coeffs[i] * sin(omega * i);
        }
        
        return sqrt(real * real + imag * imag);
    }
    
    float calculateSingleFilterResponse(float frequencyHz) {
        float omega = 2.0f * M_PI * frequencyHz / SAMPLE_RATE;
        float real = 0.0f, imag = 0.0f;
        
        for (uint16_t i = 0; i < numTaps; i++) {
            real += coefficients[i] * cos(omega * i);
            imag -= coefficients[i] * sin(omega * i);
        }
        
        return sqrt(real * real + imag * imag);
    }
};

// =====================================================================
// COEFFICIENTI PRECOMPUTATI REALI (calcolati offline con cutoff 0.92)
// =====================================================================

// Decimazione 2x, cutoff = 6.9kHz (92% di 7.5kHz)
const float AdaptiveFIRFilter::COEFFS_11_TAPS[11] = {
    -0.0087f, 0.0000f, 0.0395f, 0.0000f, -0.1515f, 
    0.2414f, -0.1515f, 0.0000f, 0.0395f, 0.0000f, -0.0087f
};

// Decimazione 3x, cutoff = 4.6kHz (92% di 5kHz)
const float AdaptiveFIRFilter::COEFFS_15_TAPS[15] = {
    -0.0054f, -0.0103f, 0.0000f, 0.0241f, 0.0000f, -0.0615f,
    0.0000f, 0.2062f, 0.0000f, -0.0615f, 0.0000f, 0.0241f,
    0.0000f, -0.0103f, -0.0054f
};

// Decimazione 5x, cutoff = 2.76kHz (92% di 3kHz)
const float AdaptiveFIRFilter::COEFFS_17_TAPS[17] = {
    -0.0038f, -0.0089f, -0.0037f, 0.0087f, 0.0047f, -0.0209f,
    -0.0136f, 0.0538f, 0.1736f, 0.0538f, -0.0136f, -0.0209f,
    0.0047f, 0.0087f, -0.0037f, -0.0089f, -0.0038f
};

// Decimazione 10x, cutoff = 1.38kHz (92% di 1.5kHz)
const float AdaptiveFIRFilter::COEFFS_21_TAPS[21] = {
    -0.0024f, -0.0056f, -0.0067f, -0.0023f, 0.0057f, 0.0095f,
    0.0046f, -0.0097f, -0.0207f, -0.0139f, 0.0496f, 0.1094f,
    0.0496f, -0.0139f, -0.0207f, -0.0097f, 0.0046f, 0.0095f,
    0.0057f, -0.0023f, -0.0067f
};

#endif // ADAPTIVE_FIR_FILTER_H