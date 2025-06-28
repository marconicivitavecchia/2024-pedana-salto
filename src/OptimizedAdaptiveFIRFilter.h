#ifndef OPTIMIZED_ADAPTIVE_FIR_FILTER_H
#define OPTIMIZED_ADAPTIVE_FIR_FILTER_H

#include <cstdint>
#include <cmath>
#include <algorithm>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ====================================================================
// OTTIMIZZAZIONI PROPOSTE PER IL FILTRO FIR ADATTIVO
// ====================================================================

class OptimizedAdaptiveFIRFilter {
public:
    enum InputType {
        ADC_UNIPOLAR,
        BIPOLAR_CENTERED
    };

private:
    static const uint16_t MAX_TAPS = 101;
    static const uint32_t SAMPLE_RATE = 30000;
    
    // === OTTIMIZZAZIONE 1: FILTRI A CASCATA ===
    struct CascadeStage {
        float coeffs[15];  // Max 15 taps per stage
        float delay[15];
        uint8_t numTaps;
        uint16_t decimation;
        uint16_t delayIndex;
        uint32_t sampleCounter;
    };
    
    CascadeStage cascadeStages[4];  // Max 4 stadi in cascata
    uint8_t numStages;
    bool useCascade;
    
    // === OTTIMIZZAZIONE 2: COEFFICIENTI PRECOMPUTATI ===
    // Tabelle per decimazioni comuni con coefficienti ottimizzati
    static const float COEFFS_11_TAPS[11];
    static const float COEFFS_21_TAPS[21]; 
    static const float COEFFS_31_TAPS[31];
    static const float COEFFS_41_TAPS[41];
    
    // === OTTIMIZZAZIONE 3: FILTRO SIMMETRICO ===
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

    // === METODI DI OTTIMIZZAZIONE ===
    
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

    // === OTTIMIZZAZIONE 1: DECOMPOSIZIONE A CASCATA ===
    void setupCascadeFilter(uint16_t totalDecimation) {
        numStages = 0;
        
        // Strategia: decomporre decimazioni grandi in fattori piccoli
        uint16_t remaining = totalDecimation;
        
        // Prima priorità: fattori di 2 (più efficienti)
        while (remaining % 2 == 0 && remaining > 1 && numStages < 4) {
            uint16_t stageDec = std::min(remaining, (uint16_t)10);
            if (stageDec > 4) stageDec = 4;  // Limita decimazione per stage
            
            cascadeStages[numStages].decimation = stageDec;
            cascadeStages[numStages].numTaps = 11;  // Sempre 11 taps per stage 2x
            cascadeStages[numStages].sampleCounter = 0;
            cascadeStages[numStages].delayIndex = 0;
            
            // Usa coefficienti precompilati
            memcpy(cascadeStages[numStages].coeffs, COEFFS_11_TAPS, 11 * sizeof(float));
            
            remaining /= stageDec;
            numStages++;
        }
        
        // Fattori di 3 e 5 se necessario
        while (remaining > 1 && numStages < 4) {
            uint16_t stageDec;
            
            if (remaining % 3 == 0) {
                stageDec = 3;
                cascadeStages[numStages].numTaps = 15;
            } else if (remaining % 5 == 0) {
                stageDec = 5;
                cascadeStages[numStages].numTaps = 15;
            } else {
                stageDec = remaining;  // Resto come ultimo stage
                cascadeStages[numStages].numTaps = std::min(remaining, (uint16_t)15);
            }
            
            cascadeStages[numStages].decimation = stageDec;
            cascadeStages[numStages].sampleCounter = 0;
            cascadeStages[numStages].delayIndex = 0;
            
            // Genera coefficienti semplificati per lo stage
            generateSimpleCoeffs(cascadeStages[numStages].coeffs, 
                               cascadeStages[numStages].numTaps, stageDec);
            
            remaining /= stageDec;
            numStages++;
        }
        
        printf("Cascade setup: %d stages for decimation %d\n", numStages, totalDecimation);
    }
    
    void generateSimpleCoeffs(float* coeffs, uint8_t taps, uint16_t decimation) {
        // Coefficienti semplificati basati su media mobile + correzione
        float sum = 0.0f;
        
        for (uint8_t i = 0; i < taps; i++) {
            // Media mobile con correzione sinc
            float weight = 1.0f;
            int center = taps / 2;
            int offset = i - center;
            
            if (offset != 0) {
                float x = M_PI * offset / decimation;
                weight = sin(x) / x;  // Correzione sinc semplificata
            }
            
            coeffs[i] = weight;
            sum += weight;
        }
        
        // Normalizza
        for (uint8_t i = 0; i < taps; i++) {
            coeffs[i] /= sum;
        }
    }
    
    bool processCascadeSample(float input, float& output) {
        float stageOutput = input;
        bool hasOutput = true;
        
        for (uint8_t stage = 0; stage < numStages; stage++) {
            if (!hasOutput) break;
            
            // Inserisci in delay line dello stage
            CascadeStage& s = cascadeStages[stage];
            s.delay[s.delayIndex] = stageOutput;
            s.delayIndex = (s.delayIndex + 1) % s.numTaps;
            
            // Calcola output FIR per questo stage
            float filtered = 0.0f;
            uint8_t tapIdx = s.delayIndex;
            
            for (uint8_t i = 0; i < s.numTaps; i++) {
                tapIdx = (tapIdx == 0) ? (s.numTaps - 1) : (tapIdx - 1);
                filtered += s.delay[tapIdx] * s.coeffs[i];
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

    // === OTTIMIZZAZIONE 2: CALCOLO SIMMETRICO ===
    bool processSymmetricSample(float input, float& output) {
        // Inserisci nella linea di ritardo
        delayLine[delayIndex] = input;
        delayIndex = (delayIndex + 1) % numTaps;
        
        // Sfrutta la simmetria dei coefficienti per dimezzare le moltiplicazioni
        float filteredOutput = 0.0f;
        uint16_t center = numTaps / 2;
        
        // Coefficiente centrale (se numTaps è dispari)
        if (numTaps % 2 == 1) {
            uint16_t centerIdx = (delayIndex + center) % numTaps;
            filteredOutput += delayLine[centerIdx] * coefficients[center];
        }
        
        // Coefficienti simmetrici (dimezza il calcolo)
        uint16_t symmetricPairs = numTaps / 2;
        for (uint16_t i = 0; i < symmetricPairs; i++) {
            uint16_t idx1 = (delayIndex + i) % numTaps;
            uint16_t idx2 = (delayIndex + numTaps - 1 - i) % numTaps;
            
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

    // === OTTIMIZZAZIONE 3: ALGORITMO TAPS INTELLIGENTE ===
    uint16_t calculateIntelligentTaps(uint16_t decimationFactor) {
        // Per decimazioni > 30, usa cascata invece di filtro monolitico
        if (decimationFactor > 30) {
            return 0;  // Segnala di usare cascata
        }
        
        // Formula ottimizzata basata su analisi prestazioni/qualità
        uint16_t baseTaps;
        
        if (decimationFactor <= 2) {
            baseTaps = 11;
        } else if (decimationFactor <= 5) {
            baseTaps = 15 + (decimationFactor - 2) * 3;  // Crescita più lenta
        } else if (decimationFactor <= 15) {
            baseTaps = 21 + (decimationFactor - 5) * 2;  // Crescita moderata
        } else {
            baseTaps = 41 + (decimationFactor - 15);     // Crescita lineare
        }
        
        // Penalità ridotta per decimazioni dispari (solo per casi critici)
        if (decimationFactor > 2 && decimationFactor % 2 == 1 && decimationFactor <= 15) {
            baseTaps += 6;  // Penalità ridotta
        }
        
        // Assicura dispari per simmetria
        if (baseTaps % 2 == 0) baseTaps++;
        
        return std::min(baseTaps, (uint16_t)MAX_TAPS);
    }

public:
    OptimizedAdaptiveFIRFilter(InputType type = ADC_UNIPOLAR, uint32_t bitDepth = 24) 
        : inputType(type), inputBitDepth(bitDepth), useCascade(false), 
          useSymmetricOptimization(true), numStages(0) {
        updateInputParameters();
        reset();
        setDecimationFactor(1);
    }

    ~OptimizedAdaptiveFIRFilter() = default;

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
        
        for (uint8_t s = 0; s < 4; s++) {
            cascadeStages[s].sampleCounter = 0;
            cascadeStages[s].delayIndex = 0;
            for (uint8_t i = 0; i < 15; i++) {
                cascadeStages[s].delay[i] = 0.0f;
                cascadeStages[s].coeffs[i] = 0.0f;
            }
        }
    }

    void setDecimationFactor(uint16_t decimationFactor) {
        if (decimationFactor == currentDecimation) return;
        
        currentDecimation = decimationFactor;
        sampleCounter = 0;
        
        if (decimationFactor == 1) {
            numTaps = 1;
            coefficients[0] = 1.0f;
            useCascade = false;
            return;
        }
        
        // DECISIONE INTELLIGENTE: Cascata vs Filtro Singolo
        uint16_t requiredTaps = calculateIntelligentTaps(decimationFactor);
        
        if (requiredTaps == 0 || decimationFactor > 30) {
            // Usa decomposizione a cascata per decimazioni elevate
            setupCascadeFilter(decimationFactor);
            useCascade = true;
            printf("Using CASCADE filter for decimation %d (%d stages)\n", 
                   decimationFactor, numStages);
        } else {
            // Usa filtro singolo ottimizzato
            useCascade = false;
            numTaps = requiredTaps;
            
            // Genera coefficienti (versione esistente)
            float cutoffFreq = (float)SAMPLE_RATE / (2.0f * decimationFactor) * 0.8f;
            generateLowpassCoefficients(cutoffFreq, numTaps);
            
            printf("Using SINGLE filter: %d taps for decimation %d\n", 
                   numTaps, decimationFactor);
        }
    }

    bool processSample(uint32_t inputSample, uint32_t& outputSample) {
        float floatInput = convertInputToFloat(inputSample);
        
        if (currentDecimation == 1) {
            outputSample = inputSample;
            return true;
        }
        
        float filteredOutput;
        bool hasOutput;
        
        if (useCascade) {
            hasOutput = processCascadeSample(floatInput, filteredOutput);
        } else if (useSymmetricOptimization && numTaps > 21) {
            hasOutput = processSymmetricSample(floatInput, filteredOutput);
        } else {
            // Elaborazione standard per filtri piccoli
            delayLine[delayIndex] = floatInput;
            delayIndex = (delayIndex + 1) % numTaps;
            
            filteredOutput = 0.0f;
            uint16_t tapIndex = delayIndex;
            
            for (uint16_t i = 0; i < numTaps; i++) {
                tapIndex = (tapIndex == 0) ? (numTaps - 1) : (tapIndex - 1);
                filteredOutput += delayLine[tapIndex] * coefficients[i];
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
    void enableSymmetricOptimization(bool enable) {
        useSymmetricOptimization = enable;
    }
    
    void getFilterInfo(uint16_t& taps, float& cutoffHz, uint32_t& outputRateHz) {
        if (useCascade) {
            taps = 0;  // Indica cascata
            for (uint8_t i = 0; i < numStages; i++) {
                taps += cascadeStages[i].numTaps;
            }
        } else {
            taps = numTaps;
        }
        
        cutoffHz = (float)SAMPLE_RATE / (2.0f * currentDecimation) * 0.8f;
        outputRateHz = SAMPLE_RATE / currentDecimation;
    }
    
    bool isUsingCascade() const { return useCascade; }
    uint8_t getNumCascadeStages() const { return numStages; }

private:
    void generateLowpassCoefficients(float cutoffFreq, uint16_t taps) {
        float omega_c = 2.0f * M_PI * cutoffFreq / SAMPLE_RATE;
        int center = taps / 2;
        float sum = 0.0f;
        
        for (int i = 0; i < taps; i++) {
            int n = i - center;
            
            if (n == 0) {
                coefficients[i] = omega_c / M_PI;
            } else {
                coefficients[i] = sin(omega_c * n) / (M_PI * n);
            }
            
            if (isnan(coefficients[i]) || isinf(coefficients[i])) {
                coefficients[i] = 0.0f;
            }
            
            // Finestra di Hamming
            float hamming = 0.54f - 0.46f * cos(2.0f * M_PI * i / (taps - 1));
            coefficients[i] *= hamming;
            sum += coefficients[i];
        }
        
        if (sum <= 0.0f || isnan(sum) || isinf(sum)) {
            for (int i = 0; i < taps; i++) {
                coefficients[i] = 1.0f / (float)taps;
            }
            return;
        }
        
        for (int i = 0; i < taps; i++) {
            coefficients[i] /= sum;
        }
    }
};

// === COEFFICIENTI PRECOMPUTATI (da definire nel .cpp) ===
const float OptimizedAdaptiveFIRFilter::COEFFS_11_TAPS[11] = {
    // Coefficienti ottimizzati per decimazione 2x (esempio)
    -0.01f, 0.02f, -0.05f, 0.12f, -0.23f, 0.64f, -0.23f, 0.12f, -0.05f, 0.02f, -0.01f
};

const float OptimizedAdaptiveFIRFilter::COEFFS_21_TAPS[21] = {
    // Coefficienti per decimazione 3x (da calcolare offline)
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f,  // Placeholder - calcolare offline
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

const float OptimizedAdaptiveFIRFilter::COEFFS_31_TAPS[31] = {
    // Coefficienti per decimazione 5x (da calcolare offline)
    // Placeholder array
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

const float OptimizedAdaptiveFIRFilter::COEFFS_41_TAPS[41] = {
    // Coefficienti per decimazione 10x (da calcolare offline)
    // Placeholder array - riempire con coefficienti ottimizzati
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f,  // Centro
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

#endif // OPTIMIZED_ADAPTIVE_FIR_FILTER_H