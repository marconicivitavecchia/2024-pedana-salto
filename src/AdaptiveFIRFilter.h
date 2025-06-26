#ifndef ADAPTIVE_FIR_FILTER_H
#define ADAPTIVE_FIR_FILTER_H

#include <cstdint>
#include <cmath>
#include <algorithm>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ====================================================================
// CLASSE FIR ADATTIVO PER DOWNSAMPLING CON ANTI-ALIASING
// ====================================================================

class AdaptiveFIRFilter {
public:
    // Enum per il tipo di input
    enum InputType {
        ADC_UNIPOLAR,        // Input da ADC unipolare (0 to MAX, centro = MAX/2)
        BIPOLAR_CENTERED     // Input bipolare centrato su zero (-MAX/2 to +MAX/2)
    };

private:
    static const uint16_t MAX_TAPS = 101;        // Numero massimo di tap (dispari per simmetria)
    static const uint32_t SAMPLE_RATE = 30000;   // Frequenza di campionamento fissa
    
    float coefficients[MAX_TAPS];                // Coefficienti del filtro
    float delayLine[MAX_TAPS];                   // Linea di ritardo (buffer circolare)
    uint16_t numTaps;                            // Numero di tap attualmente usati
    uint16_t delayIndex;                         // Indice per buffer circolare
    uint16_t currentDecimation;                  // Fattore di decimazione corrente
    uint32_t sampleCounter;                      // Contatore per decimazione
    InputType inputType;                         // Tipo di input configurato
    uint32_t inputBitDepth;                      // Profondità in bit dell'input
    float inputOffset;                           // Offset per centrare il segnale
    float inputScale;                            // Scala per normalizzare

    // Metodi privati
    void updateInputParameters() {
        if (inputType == ADC_UNIPOLAR) {
            // Input da ADC: 0 to 2^N-1, centro = 2^(N-1)
            uint32_t maxValue = (1UL << inputBitDepth) - 1;
            inputOffset = (float)maxValue / 2.0f;
            inputScale = 2.0f / (float)maxValue;  // Normalizza a ±1.0
        } else {
            // Input bipolare: -2^(N-1) to +2^(N-1)-1
            // Per 24-bit: -8388608 to +8388607
            inputOffset = 0.0f;  // Non serve offset, già centrato
            float maxMagnitude = (float)(1UL << (inputBitDepth - 1));  // 2^(N-1)
            inputScale = 1.0f / maxMagnitude;  // Normalizza a ±1.0
        }
    }

    float convertInputToFloat(uint32_t inputSample) {
        if (inputType == ADC_UNIPOLAR) {
            // Centra attorno a zero e normalizza
            return ((float)inputSample - inputOffset) * inputScale;
        } else {
            // Input bipolare: interpreta come signed integer
            // Per 24-bit: range ±8388608 (±2^23)
            int32_t signedInput = (int32_t)inputSample;
            
            // Se il valore è maggiore del max positivo, è negativo (two's complement)
            int32_t maxPositive = (1L << (inputBitDepth - 1)) - 1;
            if (signedInput > maxPositive) {
                signedInput -= (1UL << inputBitDepth);
            }
            
            return (float)signedInput * inputScale;
        }
    }

    uint32_t convertFloatToOutput(float floatValue) {
        if (inputType == ADC_UNIPOLAR) {
            // Denormalizza e ricentra per output ADC
            float reconverted = (floatValue / inputScale) + inputOffset;
            
            // Clamp ai limiti dell'ADC
            uint32_t maxValue = (1UL << inputBitDepth) - 1;
            if (reconverted < 0.0f) {
                return 0;
            } else if (reconverted > (float)maxValue) {
                return maxValue;
            } else {
                return (uint32_t)reconverted;
            }
        } else {
            // Denormalizza per output bipolare
            float reconverted = floatValue / inputScale;
            
            // Clamp ai limiti bipolari: ±2^(N-1) 
            // Per 24-bit: -8388608 a +8388607
            int32_t maxValue = (1L << (inputBitDepth - 1)) - 1;        // +2^23-1
            int32_t minValue = -(1L << (inputBitDepth - 1));           // -2^23
            
            int32_t result;
            if (reconverted < (float)minValue) {
                result = minValue;
            } else if (reconverted > (float)maxValue) {
                result = maxValue;
            } else {
                result = (int32_t)reconverted;
            }
            
            // Converti signed in unsigned per il return
            return (uint32_t)result;
        }
    }

    float calculateCutoffFrequency(uint16_t decimationFactor) {
        // Frequenza di Nyquist dopo decimazione
        float nyquistAfterDecimation = (float)SAMPLE_RATE / (2.0f * decimationFactor);
        
        // Usa 80% della frequenza di Nyquist per margine di sicurezza (normale per tutti)
        return nyquistAfterDecimation * 0.8f;
    }

    uint16_t calculateOptimalTaps(uint16_t decimationFactor) {
		// Tap base teoricamente corretti (crescita monotonica)
		uint16_t baseTaps;
		
		switch (decimationFactor) {
			case 1:
				baseTaps = 1;   // Pass-through
				break;
			case 2:
				baseTaps = 11;  // 30kSps → 15kSps (pari, facile)
				break;
			case 3:
				baseTaps = 21;  // 30kSps → 10kSps (dispari, problematico)
				break;
			case 5:
				baseTaps = 31;  // 30kSps → 6kSps (dispari, problematico)
				break;
			case 10:
				baseTaps = 41;  // 30kSps → 3kSps (pari, più facile)
				break;
			case 15:
				baseTaps = 51;  // 30kSps → 2kSps (dispari, problematico)
				break;
			case 30:
				baseTaps = 71;  // 30kSps → 1kSps (pari, ma alta decimazione)
				break;
			case 50:
				baseTaps = 81;  // 30kSps → 600Hz (pari, estremo)
				break;
			case 100:
				baseTaps = 91;  // 30kSps → 300Hz (pari, estremo)
				break;
			case 150:
				baseTaps = 101; // 30kSps → 200Hz (pari, massimo)
				break;
			default:
				// Formula empirica per decimazioni non previste
				if (decimationFactor <= 2) {
					baseTaps = 11;
				} else if (decimationFactor <= 5) {
					baseTaps = 21 + (decimationFactor - 3) * 5;
				} else if (decimationFactor <= 20) {
					baseTaps = 31 + (decimationFactor - 5) * 2;
				} else if (decimationFactor <= 60) {
					baseTaps = 51 + (decimationFactor - 20);
				} else {
					baseTaps = std::min(101, 71 + (decimationFactor - 60) / 2);
				}
				break;
		}
		
		// PENALITÀ PER DECIMAZIONI DISPARI
		// Le decimazioni dispari causano aliasing asimmetrico e spike
		if (decimationFactor > 1 && decimationFactor % 2 == 1) {
			uint16_t penalty;
			
			if (decimationFactor <= 5) {
				penalty = 20;  // Penalità alta per dispari piccoli (3x, 5x)
			} else if (decimationFactor <= 15) {
				penalty = 15;  // Penalità media per dispari medi
			} else {
				penalty = 10;  // Penalità ridotta per dispari alti
			}
			
			baseTaps += penalty;
			printf("Odd decimation %d: added %d penalty taps (total: %d)\n", 
				   decimationFactor, penalty, baseTaps);
		}
		
		// Assicura che sia dispari per simmetria
		if (baseTaps % 2 == 0) baseTaps++;
		
		return std::min(baseTaps, (uint16_t)MAX_TAPS);
	}

    void generateLowpassCoefficients(float cutoffFreq, uint16_t taps) {
        float omega_c = 2.0f * M_PI * cutoffFreq / SAMPLE_RATE;  // Frequenza normalizzata
        int center = taps / 2;
        
        float sum = 0.0f;  // Per normalizzazione
        
        for (int i = 0; i < taps; i++) {
            int n = i - center;
            
            if (n == 0) {
                // Caso speciale per n=0 (evita divisione per zero)
                coefficients[i] = omega_c / M_PI;
            } else {
                // Formula sinc per filtro ideale
                coefficients[i] = sin(omega_c * n) / (M_PI * n);
            }
            
            // Verifica che il coefficiente sia valido
            if (isnan(coefficients[i]) || isinf(coefficients[i])) {
                coefficients[i] = 0.0f;  // Safety fallback
            }
            
            // Applica finestra di Hamming per ridurre ripple
            float hamming = 0.54f - 0.46f * cos(2.0f * M_PI * i / (taps - 1));
            coefficients[i] *= hamming;
            
            sum += coefficients[i];
        }
        
        // Verifica che sum sia valida
        if (sum <= 0.0f || isnan(sum) || isinf(sum)) {
            // Fallback: coefficienti unitari
            for (int i = 0; i < taps; i++) {
                coefficients[i] = 1.0f / (float)taps;
            }
            return;
        }
        
        // Normalizza per guadagno unitario in DC
        for (int i = 0; i < taps; i++) {
            coefficients[i] /= sum;
        }
    }

public:
    // Costruttore con parametri opzionali
    AdaptiveFIRFilter(InputType type = ADC_UNIPOLAR, uint32_t bitDepth = 24) 
        : inputType(type), inputBitDepth(bitDepth) {
        updateInputParameters();
        reset();
        setDecimationFactor(1);  // Inizializza con decimazione = 1
    }

    // Distruttore
    ~AdaptiveFIRFilter() = default;

    // Reset completo del filtro
    void reset() {
        delayIndex = 0;
        sampleCounter = 0;
        currentDecimation = 1;
        numTaps = 1;
        
        // Azzera la linea di ritardo e coefficienti
        for (uint16_t i = 0; i < MAX_TAPS; i++) {
            delayLine[i] = 0.0f;
            coefficients[i] = 0.0f;
        }
    }

    // Configura il tipo di input e profondità in bit
    void setInputType(InputType type, uint32_t bitDepth = 24) {
        inputType = type;
        inputBitDepth = bitDepth;
        updateInputParameters();
        
        printf("FIR: InputType=%s, BitDepth=%lu, Offset=%.1f, Scale=%.6f\n",
               (type == ADC_UNIPOLAR) ? "ADC_UNIPOLAR" : "BIPOLAR_CENTERED",
               bitDepth, inputOffset, inputScale);
    }

    // Configura il filtro per un nuovo fattore di decimazione
    void setDecimationFactor(uint16_t decimationFactor) {
        if (decimationFactor == currentDecimation) return;  // Nessun cambiamento
        
        currentDecimation = decimationFactor;
        sampleCounter = 0;
        
        // Per decimazione = 1, usa filtro pass-through
        if (decimationFactor == 1) {
            numTaps = 1;
            coefficients[0] = 1.0f;
            return;
        }
        
        // Calcola parametri ottimali
        float cutoffFreq = calculateCutoffFrequency(decimationFactor);
        numTaps = calculateOptimalTaps(decimationFactor);
        
        // Genera nuovi coefficienti
        generateLowpassCoefficients(cutoffFreq, numTaps);
    }

    // Elabora un campione e restituisce true se c'è output (dopo decimazione)
    bool processSample(uint32_t inputSample, uint32_t& outputSample) {
        // Converti l'input al formato float normalizzato
        float floatInput = convertInputToFloat(inputSample);
        
        // Per decimazione = 1, passa direttamente (ottimizzazione)
        if (currentDecimation == 1) {
            outputSample = inputSample;
            return true;
        }
        
        // Inserisci nella linea di ritardo (buffer circolare)
        delayLine[delayIndex] = floatInput;
        delayIndex = (delayIndex + 1) % numTaps;
        
        // Calcola l'uscita del filtro FIR (convoluzione)
        float filteredOutput = 0.0f;
        uint16_t tapIndex = delayIndex;
        
        for (uint16_t i = 0; i < numTaps; i++) {
            // Indice circolare all'indietro
            tapIndex = (tapIndex == 0) ? (numTaps - 1) : (tapIndex - 1);
            filteredOutput += delayLine[tapIndex] * coefficients[i];
        }
        
        // Incrementa contatore per decimazione
        sampleCounter++;
        
        // Verifica se è tempo di produrre un output decimato
        if (sampleCounter >= currentDecimation) {
            sampleCounter = 0;
            
            // Riconverti al formato di output
            outputSample = convertFloatToOutput(filteredOutput);
            
            return true;  // C'è un nuovo campione di output
        }
        
        return false;  // Nessun output questo ciclo
    }

    // Funzione di utilità per ottenere info sul filtro
    void getFilterInfo(uint16_t& taps, float& cutoffHz, uint32_t& outputRateHz) {
        taps = numTaps;
        cutoffHz = calculateCutoffFrequency(currentDecimation);
        outputRateHz = SAMPLE_RATE / currentDecimation;
    }

    // Funzione per ottenere la risposta del filtro a una data frequenza (per debug/test)
    float getFrequencyResponse(float frequencyHz) {
        float omega = 2.0f * M_PI * frequencyHz / SAMPLE_RATE;
        float real = 0.0f, imag = 0.0f;
        
        for (uint16_t i = 0; i < numTaps; i++) {
            real += coefficients[i] * cos(omega * i);
            imag -= coefficients[i] * sin(omega * i);
        }
        
        return sqrt(real * real + imag * imag);  // Magnitudine
    }

    // Getters
    uint16_t getCurrentDecimation() const { return currentDecimation; }
    uint16_t getNumTaps() const { return numTaps; }
    uint32_t getSampleRate() const { return SAMPLE_RATE; }
    InputType getInputType() const { return inputType; }
    uint32_t getInputBitDepth() const { return inputBitDepth; }
    float getInputOffset() const { return inputOffset; }
    float getInputScale() const { return inputScale; }
};

#endif // ADAPTIVE_FIR_FILTER_H