#ifndef ADS1256_DMA_H
#define ADS1256_DMA_H

#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <Arduino.h>
#include "esp_log.h"
#include "ADS1256_delays.h"

#include "soc/spi_struct.h"
#include "soc/spi_reg.h"
#include "soc/soc.h"

// Pin definitions
#define ADS1256_PIN_CS   5
#define ADS1256_PIN_DRDY 4
#define ADS1256_PIN_MISO 19 // DOUT
#define ADS1256_PIN_MOSI 23 // DIN
#define ADS1256_PIN_SCK  18

// ADS1256 Register definitions
#define ADS1256_REG_STATUS  0x00
#define ADS1256_REG_MUX     0x01
#define ADS1256_REG_ADCON   0x02
#define ADS1256_REG_DRATE   0x03
#define ADS1256_REG_IO      0x04
#define ADS1256_REG_OFC0    0x05
#define ADS1256_REG_OFC1    0x06
#define ADS1256_REG_OFC2    0x07
#define ADS1256_REG_FSC0    0x08
#define ADS1256_REG_FSC1    0x09
#define ADS1256_REG_FSC2    0x0A

// Commands
#define ADS1256_CMD_WAKEUP  0x00
#define ADS1256_CMD_RDATA   0x01
#define ADS1256_CMD_RDATAC  0x03
#define ADS1256_CMD_SDATAC  0x0F
#define ADS1256_CMD_RREG    0x10
#define ADS1256_CMD_WREG    0x50
#define ADS1256_CMD_SELFCAL 0xF0
#define ADS1256_CMD_SELFOCAL 0xF1
#define ADS1256_CMD_SELFGCAL 0xF2
#define ADS1256_CMD_SYSOCAL 0xF3
#define ADS1256_CMD_SYSGCAL 0xF4
#define ADS1256_CMD_SYNC    0xFC
#define ADS1256_CMD_STANDBY 0xFD
#define ADS1256_CMD_RESET   0xFE

#define MAX_SAMPLES_PER_BATCH 310u  // Aggiunto 'u' per unsigned

// Chip settings
enum ads1256_channels_t {
    ADS1256_AIN0 = 0,
    ADS1256_AIN1 = 1,
    ADS1256_AIN2 = 2,
    ADS1256_AIN3 = 3,
    ADS1256_AIN4 = 4,
    ADS1256_AIN5 = 5,
    ADS1256_AIN6 = 6,
    ADS1256_AIN7 = 7,
    ADS1256_AINCOM = 8
};

enum ads1256_gain_t {
    ADS1256_GAIN_1 = 0,   
    ADS1256_GAIN_2 = 1,   
    ADS1256_GAIN_4 = 2,   
    ADS1256_GAIN_8 = 3,   
    ADS1256_GAIN_16 = 4,  
    ADS1256_GAIN_32 = 5,  
    ADS1256_GAIN_64 = 6   
};

// Struttura per il batch di campioni
struct BatchData {
    uint32_t timestamp;
    //int32_t first;
    uint8_t count;
    uint8_t values[MAX_SAMPLES_PER_BATCH][3];  // 3 bytes per valore
};

struct ADS1256Status {
   uint8_t status;      // STATUS register
   uint8_t mux;         // MUX register
   uint8_t adcon;       // ADCON register 
   uint8_t drate;       // DRATE register
   bool rdatac_active;  // True se in RDATAC
   bool chip_active;    // True se chip risponde
};

// Primo: definire i possibili codici di errore
enum class ADS1256_Error {
    NO_ERROR = 0,
    SPI_INIT_FAILED,
    CALIBRATION_TIMEOUT,
    CALIBRATION_FAILED,
    INVALID_CHANNEL,
    INVALID_GAIN,
    COMMUNICATION_ERROR,
    BUS_ACQUISITION_FAILED,
    DRDY_TIMEOUT,
    INVALID_REGISTER,
    BUFFER_OVERFLOW
};

// Secondo: aggiungere una classe per la gestione degli errori
class ADS1256_ErrorHandler {
private:
    ADS1256_Error lastError;
    char errorMessage[100];
    
public:
    ADS1256_ErrorHandler() : lastError(ADS1256_Error::NO_ERROR) {
        errorMessage[0] = '\0';
    }
    
    void setError(ADS1256_Error error, const char* message) {
        lastError = error;
        strncpy(errorMessage, message, sizeof(errorMessage) - 1);
        errorMessage[sizeof(errorMessage) - 1] = '\0';
        
        // Log dell'errore
        if(error != ADS1256_Error::NO_ERROR) {
            Serial.printf("ADS1256 Error: %s\n", message);
        }
    }
    
    ADS1256_Error getLastError() const { return lastError; }
    const char* getErrorMessage() const { return errorMessage; }
    void clearError() { 
        lastError = ADS1256_Error::NO_ERROR; 
        errorMessage[0] = '\0';
    }
};


class ADS1256_DMA {
public:

    ADS1256_DMA() : emaFilteredValue(0.0f) {
        Serial.println("Costruttore: inizio");
        
        // Setup delle direzioni GPIO con pull-up/down appropriati
        gpio_config_t io_conf = {};
       
        // CS come output, inizialmente alto
        pinMode(ADS1256_PIN_CS, OUTPUT);
        digitalWrite(ADS1256_PIN_CS, HIGH);
        //gpio_set_direction((gpio_num_t)ADS1256_PIN_CS, GPIO_MODE_OUTPUT);
        //gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 1);
        pinMode(ADS1256_PIN_DRDY, INPUT);  // Input semplice senza pull-up
        //gpio_set_direction((gpio_num_t)ADS1256_PIN_DRDY, GPIO_MODE_INPUT);
        // Set PULL UP for DRDY Pin
	    //gpio_set_pull_mode((gpio_num_t)ADS1256_PIN_DRDY, GPIO_FLOATING);

        /*
        // DRDY come input con pull-up
        io_conf.pin_bit_mask = (1ULL << ADS1256_PIN_DRDY);
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.intr_type = GPIO_INTR_DISABLE;
        gpio_config(&io_conf);
        */

        pinMode(ADS1256_PIN_MISO, INPUT);       // MISO (input da ADS1256)
        pinMode(ADS1256_PIN_MOSI, OUTPUT);      // MOSI (output verso ADS1256)
        pinMode(ADS1256_PIN_SCK, OUTPUT);       // SCLK (clock generato da ESP32)
        // MISO come input
        //gpio_set_direction((gpio_num_t)ADS1256_PIN_MISO, GPIO_MODE_INPUT);
        
        // MOSI e SCLK come output
        //gpio_set_direction((gpio_num_t)ADS1256_PIN_MOSI, GPIO_MODE_OUTPUT);
        //gpio_set_direction((gpio_num_t)ADS1256_PIN_SCK, GPIO_MODE_OUTPUT);

        // Setup SPI configuration
        spi_bus_config_t buscfg;
        memset(&buscfg, 0, sizeof(spi_bus_config_t));
            
        buscfg.mosi_io_num = ADS1256_PIN_MOSI;
        buscfg.miso_io_num = ADS1256_PIN_MISO;
        buscfg.sclk_io_num = ADS1256_PIN_SCK;
        buscfg.quadwp_io_num = -1;
        buscfg.quadhd_io_num = -1;
        buscfg.max_transfer_sz = 32;
        buscfg.flags = 0;

        // Configurazione dispositivo SPI
        // spi_device_interface_config_t devcfg;
        // memset(&devcfg, 0, sizeof(spi_device_interface_config_t));

        // Riconfigura SPI più lento
        spi_device_interface_config_t devcfg = {};
        devcfg.mode = 1;                    // CPOL=0, CPHA=1
        devcfg.clock_speed_hz = 1000000;     // Ridotto a 100kHz per debug
        devcfg.spics_io_num = -1;           // CS manuale
        devcfg.queue_size = 1;
        devcfg.flags = SPI_DEVICE_NO_DUMMY;

        devcfg.command_bits = 0;
        devcfg.address_bits = 0;
        devcfg.dummy_bits = 0;
        devcfg.duty_cycle_pos = 128;        // Default duty cycle
        devcfg.cs_ena_pretrans = 0;
        devcfg.cs_ena_posttrans = 0;
        devcfg.pre_cb = NULL;
        devcfg.post_cb = NULL;

        if (!spi_initialized) {
            Serial.println("ADS1256_DMA: inizializzo SPI bus...");
            
            esp_err_t ret = spi_bus_initialize(VSPI_HOST, &buscfg, 1);
            if (ret != ESP_OK) {
                Serial.print("Errore nell'inizializzazione del bus SPI: ");
                Serial.println(esp_err_to_name(ret));  // Converte l'errore in una stringa leggibile
                return;  // Esci dalla funzione in caso di errore
            }
            Serial.println("ADS1256_DMA: SPI bus inizializzato");

            Serial.println("ADS1256_DMA: aggiungo device SPI...");
            ret = spi_bus_add_device(VSPI_HOST, &devcfg, &spi);
            if (ret != ESP_OK) {
                Serial.print("Errore nell'aggiunta del device SPI: ");
                Serial.println(esp_err_to_name(ret));
                return;  // Esci dalla funzione in caso di errore
            }
            Serial.println("ADS1256_DMA: device SPI aggiunto");
        
            // update static flag
            spi_initialized = true;
        }   

        esp_err_t ret = spi_device_acquire_bus(spi, portMAX_DELAY);
        if(ret == ESP_OK) {
            Serial.println("Bus SPI acquisito, inizializzo ADS1256...");
            if(!init_ads1256()) {
                Serial.println("Calibrazione ADS1256 fallita!");
                errorHandler.setError(ADS1256_Error::CALIBRATION_FAILED, 
                    "ADS1256 initialization failed");
                return;
            }
            spi_device_release_bus(spi);
        }else{
            Serial.println("ADS1256_DMA: BUS non acquisito");
            //return;
        }    
    }

/*
    ADS1256_DMA() : emaFilteredValue(0.0f) {
        Serial.println("\n=== Inizio inizializzazione ADS1256 ===");

        // Reset completo di tutti i pin GPIO usati
        Serial.println("Reset pin GPIO...");
        gpio_reset_pin((gpio_num_t)ADS1256_PIN_CS);
        gpio_reset_pin((gpio_num_t)ADS1256_PIN_DRDY);
        gpio_reset_pin((gpio_num_t)ADS1256_PIN_MISO);
        gpio_reset_pin((gpio_num_t)ADS1256_PIN_MOSI);
        gpio_reset_pin((gpio_num_t)ADS1256_PIN_SCK);
        
        // Configurazione CS con pull-up hardware
        gpio_config_t io_conf = {};
        io_conf.pin_bit_mask = (1ULL << ADS1256_PIN_CS);
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.intr_type = GPIO_INTR_DISABLE;
        ESP_ERROR_CHECK(gpio_config(&io_conf));

        // Forza CS alto
        gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 1);
        Serial.printf("CS dopo setup forzato: %d\n", gpio_get_level((gpio_num_t)ADS1256_PIN_CS));
        delay(10);  // Delay più lungo per stabilizzazione

        // Verifica nuovamente
        Serial.printf("CS dopo delay: %d\n", gpio_get_level((gpio_num_t)ADS1256_PIN_CS));

        // DRDY come input senza pull-up (è già pilotato dall'ADS1256)
        Serial.println("Setup DRDY...");
        gpio_set_direction((gpio_num_t)ADS1256_PIN_DRDY, GPIO_MODE_INPUT);
        gpio_set_pull_mode((gpio_num_t)ADS1256_PIN_DRDY, GPIO_FLOATING);
        Serial.printf("DRDY level dopo setup: %d\n", gpio_get_level((gpio_num_t)ADS1256_PIN_DRDY));

        // Altri pin GPIO
        Serial.println("Setup altri pin SPI...");
        gpio_set_direction((gpio_num_t)ADS1256_PIN_MISO, GPIO_MODE_INPUT);
        gpio_set_direction((gpio_num_t)ADS1256_PIN_MOSI, GPIO_MODE_OUTPUT);
        gpio_set_direction((gpio_num_t)ADS1256_PIN_SCK, GPIO_MODE_OUTPUT);

        Serial.println("\nStato pin dopo setup iniziale:");
        Serial.printf("CS: %d\n", gpio_get_level((gpio_num_t)ADS1256_PIN_CS));
        Serial.printf("DRDY: %d\n", gpio_get_level((gpio_num_t)ADS1256_PIN_DRDY));
        Serial.printf("MISO: %d\n", gpio_get_level((gpio_num_t)ADS1256_PIN_MISO));
        Serial.printf("MOSI: %d\n", gpio_get_level((gpio_num_t)ADS1256_PIN_MOSI));
        Serial.printf("SCLK: %d\n", gpio_get_level((gpio_num_t)ADS1256_PIN_SCK));

        // Setup SPI configuration
        Serial.println("\nConfigurazione SPI...");
        spi_bus_config_t buscfg;
        memset(&buscfg, 0, sizeof(spi_bus_config_t));
            
        buscfg.mosi_io_num = ADS1256_PIN_MOSI;
        buscfg.miso_io_num = ADS1256_PIN_MISO;
        buscfg.sclk_io_num = ADS1256_PIN_SCK;
        buscfg.quadwp_io_num = -1;
        buscfg.quadhd_io_num = -1;
        buscfg.max_transfer_sz = 32;
        buscfg.flags = 0;

        // Configurazione dispositivo SPI
        spi_device_interface_config_t devcfg;
        memset(&devcfg, 0, sizeof(spi_device_interface_config_t));

        devcfg.mode = 1;                    // CPOL=0, CPHA=1
        devcfg.clock_speed_hz = 1000000;    // Ridotto a 1MHz per debug
        devcfg.spics_io_num = -1;           // CS gestito via software
        devcfg.queue_size = 1;
        devcfg.flags = SPI_DEVICE_NO_DUMMY;
        devcfg.command_bits = 0;
        devcfg.address_bits = 0;
        devcfg.dummy_bits = 0;
        devcfg.duty_cycle_pos = 128;        // Default duty cycle
        devcfg.cs_ena_pretrans = 0;
        devcfg.cs_ena_posttrans = 0;
        devcfg.pre_cb = NULL;
        devcfg.post_cb = NULL;

        if (!spi_initialized) {
            Serial.println("Inizializzo SPI bus...");
            ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &buscfg, 1));
            Serial.println("SPI bus inizializzato");
            
            Serial.println("Aggiunta device SPI...");
            ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &devcfg, &spi));
            Serial.println("Device SPI aggiunto");
            spi_initialized = true;
        }

        Serial.println("\nStato pin dopo inizializzazione SPI:");
        Serial.printf("CS: %d\n", gpio_get_level((gpio_num_t)ADS1256_PIN_CS));
        Serial.printf("DRDY: %d\n", gpio_get_level((gpio_num_t)ADS1256_PIN_DRDY));
        Serial.printf("MISO: %d\n", gpio_get_level((gpio_num_t)ADS1256_PIN_MISO));
        Serial.printf("MOSI: %d\n", gpio_get_level((gpio_num_t)ADS1256_PIN_MOSI));
        Serial.printf("SCLK: %d\n\n", gpio_get_level((gpio_num_t)ADS1256_PIN_SCK));

        esp_err_t ret = spi_device_acquire_bus(spi, portMAX_DELAY);
        if(ret == ESP_OK) {
            Serial.println("Bus SPI acquisito, inizializzo ADS1256...");
            if(!init_ads1256()) {
                Serial.println("Calibrazione ADS1256 fallita!");
                errorHandler.setError(ADS1256_Error::CALIBRATION_FAILED, 
                    "ADS1256 initialization failed");
            }
            spi_device_release_bus(spi);
        }

        Serial.println("\nStato finale pin:");
        Serial.printf("CS: %d\n", gpio_get_level((gpio_num_t)ADS1256_PIN_CS));
        Serial.printf("DRDY: %d\n", gpio_get_level((gpio_num_t)ADS1256_PIN_DRDY));
        Serial.printf("MISO: %d\n", gpio_get_level((gpio_num_t)ADS1256_PIN_MISO));
        Serial.printf("MOSI: %d\n", gpio_get_level((gpio_num_t)ADS1256_PIN_MOSI));
        Serial.printf("SCLK: %d\n", gpio_get_level((gpio_num_t)ADS1256_PIN_SCK));
        Serial.println("=== Fine inizializzazione ADS1256 ===\n");
    }
*/
    bool init_ads1256() {
        if (!spi_initialized) return false;
        
        // Hard reset
        sendCommand(ADS1256_CMD_RESET);
        T11Delay();  // 25ms delay dopo reset
        
        waitDRDY();

        // Stop continuous mode
        sendCommand(ADS1256_CMD_SDATAC);
        
        // Setup registri base
        if(!writeRegister(ADS1256_REG_STATUS, 0x03)) {   // Buffer OFF, no auto-cal
            return false;
        }
        
        if(!writeRegister(ADS1256_REG_DRATE, 0xF0)) {    // 30000 SPS
            return false;
        }
        
        if(!writeRegister(ADS1256_REG_MUX, 0x08)) {      // AIN0 vs AINCOM
            return false;
        }
        
        if(!set_gain(ADS1256_GAIN_1)) {
            return false;
        }
        
        // Avvio calibrazione
        sendCommand(ADS1256_CMD_SELFCAL);
        
        // Attendi completamento calibrazione con timeout
        if(!waitDRDY(1000)) {
            Serial.println("Timeout durante la calibrazione");
            return false;
        }
        
        // Verifica status register per conferma calibrazione
        uint8_t status = readRegister(ADS1256_REG_STATUS);
        if(status & 0x01) { // Bit 0 = Order Status Bit
            Serial.println("Calibrazione fallita - Status check");
            return false;
        }
        
        Serial.println("Inizializzazione ADS1256 completata con successo");
        return true;
    }

    ~ADS1256_DMA() {
        if(isStreaming) {
            stopStreaming();  // Questo rilascia il bus se era acquisito
        }
        
        if(spi) {
            spi_bus_remove_device(spi);
            spi_bus_free(VSPI_HOST);
        }
    }

    // Metodi pubblici per accedere agli errori
    ADS1256_Error getLastError() const { 
        return errorHandler.getLastError(); 
    }
    
    const char* getErrorMessage() const { 
        return errorHandler.getErrorMessage(); 
    }
    
    void clearErrors() { 
        errorHandler.clearError(); 
    }

    /* read_data_batch
    ---------------------------------------
        TEST SIGNAL GNERATION SECTION
    ---------------------------------------
    */
    
    uint32_t readCurrData() {
        uint32_t data = read_data();
        return data;
    }

    int32_t read_data() {
        waitDRDY();
        
        sendCommand(ADS1256_CMD_RDATA);
        T6Delay();

        CSON();
        T2Delay();

        uint8_t data[3];
        spi_transaction_t t2 = {};
        t2.length = 24;
        t2.rx_buffer = data;
        esp_err_t ret = spi_device_transmit(spi, &t2);

        T3Delay();
        CSOFF();

        if(ret != ESP_OK) {
            errorHandler.setError(ADS1256_Error::COMMUNICATION_ERROR,
                "Data read failed");
            return 0;
        }

        int32_t value = ((int32_t)data[0] << 16) |
                        ((int32_t)data[1] << 8)  |
                        (int32_t)data[2];

        if(value & 0x800000) {
            value |= 0xFF000000;
        }

        return value + offset;
    }  

    void read_data_batch(BatchData& batch, uint16_t samplesPerBatch, uint16_t decimationFactor = 1) {
        if(!isStreaming || !spi_initialized) return;
        
        batch.count = 0;
        batch.timestamp = esp_timer_get_time();
        
        CSON();
        T2Delay();

        SPI3.slave.trans_done = 0;  // resetta il flag che indica il completamento di una transazione
        SPI3.cmd.usr = 1;           // avvia una nuova transazione SPI

        uint32_t value = 0;
        while(batch.count < samplesPerBatch) {
            int32_t accumulator = 0;
            
            for(uint16_t i = 0; i < decimationFactor; i++) {
                waitDRDY();
                
                // Avvia la transazione SPI
                //SPI2.cmd.usr = 1;

                // Attendi il completamento della transazione
                //while (SPI2.cmd.usr);

                // Leggi i dati ricevuti
                value = SPI3.data_buf[0];  // 24 bit di dati

                #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
                    value >>= 8;  // Allinea i 24 bit se big endian
                #else
                    value = __builtin_bswap32(value) >> 8;  // Swap e allinea se little endian
                #endif  

                // Se il dispositivo fornisce 24 bit, i bit più significativi (31-24) sono da ignorare
                value &= 0x00FFFFFF; // Mantieni solo i 24 bit meno significativi

                // Estensione del segno, se il dato è con segno (ADS1256 fornisce dati signed)
                if (value & 0x800000) {  // Se il bit 23 è 1 (numero negativo)
                    value |= 0xFF000000; // Estendi il segno nei bit superiori
                }

                //accumulator += (value + offset);
            }
            
            emaFilteredValue = emaAlpha * (value) + 
                            (1.0f - emaAlpha) * emaFilteredValue;
            
            uint32_t output = (uint32_t)emaFilteredValue;
            batch.values[batch.count][0] = output >> 16;
            batch.values[batch.count][1] = output >> 8;
            batch.values[batch.count][2] = output;
            
            batch.count++;
        }
        
        CSOFF();
    }

    void enableTestSignal(bool enable) {
        testSignalEnabled = enable;
        if (testSignalEnabled) {
            testSignalTime = 0;
        }
    }

    void forceOffset(uint32_t val) {
        offset = val;
    }

    void setTestSignalParams(float freq, float ampl, float amFreq) {
        testFrequency = freq;
        baseAmplitude = ampl;
        amFrequency = amFreq;
    }

    void generateTestSamples(BatchData& batch, uint16_t samplesPerBatch, uint16_t decimationFactor) {
        if (!testSignalEnabled) return;

        float signalPeriod = (float) decimationFactor / 30000;
        batch.count = 0;
        batch.timestamp = esp_timer_get_time();
        float t = 1 / testFrequency;
        float amp = baseAmplitude;

        const float voltsToAdc = 8388608.0f / vRef;  // 2^23 / vRef
        
        // Incremento per ogni nuovo batch (esempio: incremento del 10% del range)
        rampValue += testFrequency*0.005f;
        // Usiamo rampValue invece della sinusoide
        float volts = rampValue * vRef;

        // Limitazione tensione
        if (volts < 0.0f) volts = 0.0f;
        //if (volts > vRef) rampValue = 0.0f;
        if (rampValue > 1.0f) rampValue = 0.0f;  // Reset quando raggiunge il massimo
        //batch.first =  (int32_t) (rampValue * 8388608.0f);
        //Serial.println(rampValue * 8388608.0f);
        //if (rampValue > 1.0f) rampValue = 0.0f;  // Reset quando raggiunge il massimo

        for (uint16_t i = 0; i < samplesPerBatch; i++) {
            
            int32_t value = (int32_t)(rampValue * amp);

            if (value < 0) {
                value += 0x1000000;
            }
            
            batch.values[i][0] = ((int32_t) value >> 16) & 0xFF;
            batch.values[i][1] = ((int32_t) value >> 8) & 0xFF;
            batch.values[i][2] = (int32_t) value & 0xFF;

            testSignalTime += signalPeriod;
            batch.count++;
        }
    }

    /* 
    ---------------------------------------
        REAL SIGNAL SAMPLING SECTION
    ---------------------------------------
    */
    float apply_ema(int32_t value) {
        emaFilteredValue = emaAlpha * value + (1.0f - emaAlpha) * emaFilteredValue;
        return emaFilteredValue;
    }  

    void startStreaming() {
        if(isStreaming || !spi_initialized) return;
        //CSON();
        // RDATAC è "continua a darmi campioni finché non ti dico di fermarti"
        sendCommand(ADS1256_CMD_RDATAC);
        isStreaming = true;
    }

    static void stopStreaming() {
        if(!isStreaming || !spi_initialized) return;
        // SDATAC è "fermati"
        sendCommand_static(ADS1256_CMD_SDATAC);  // Usa la versione statica
        //CSOFF_static();
        isStreaming = false;
    }

    // Versioni statiche per le funzioni statiche della classe
    static void CSON_static() {
        digitalWrite(ADS1256_PIN_CS, LOW);  // CS attivo basso
        //gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 0);
        T2Delay();  // Delay dopo CS basso
    }

    static void CSOFF_static() {
        T3Delay();  // Delay prima di CS alto
        //gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 1);
        digitalWrite(ADS1256_PIN_CS, HIGH); // CS inattivo alto
    }

    static void resetSPI() {
        Serial.println("resetSPI: inizio");
        
        if (!spi_initialized || spi == nullptr) {
            Serial.println("resetSPI: SPI non inizializzato o già resettato");
            return;
        }

        // 1. Prima fermiamo lo streaming se attivo
        if(isStreaming) {
             stopStreaming();
        }
        // 2. Aspettiamo che eventuali transazioni in corso si completino
        vTaskDelay(pdMS_TO_TICKS(100)); 

        // 3. Rimuoviamo il dispositivo SPI
        
        if(spi){
            //spi_device_release_bus(spi);
            Serial.println("resetSPI: removing device...");
            spi_bus_remove_device(spi);
            Serial.println("resetSPI: freeing device...");
            spi_bus_free(VSPI_HOST);
            spi = nullptr;
        }  
 
        // 5. Reset dei flag
        spi_initialized = false;
        isStreaming = false;
        
        Serial.println("resetSPI: completato");
    }

    bool set_single_channel(ads1256_channels_t channel) {
        return set_channel(channel, ADS1256_AINCOM);
    }

    void setEMAalfa(float alfa){
        emaAlpha = alfa;
    }

    bool set_channel(ads1256_channels_t positive_ch, ads1256_channels_t negative_ch = ADS1256_AINCOM) {
        if(!spi_initialized) {
            errorHandler.setError(ADS1256_Error::SPI_INIT_FAILED, 
                "SPI not initialized");
            return false;
        }
        if(positive_ch > ADS1256_AIN7 || negative_ch > ADS1256_AINCOM) {
            errorHandler.setError(ADS1256_Error::INVALID_CHANNEL, 
                "Invalid channel selection");
            return false;
        }
        /* 
        // Acquisizione condizionale del bus
        if(need_acquire) {
            esp_err_t ret = spi_device_acquire_bus(spi, portMAX_DELAY);
            if(ret != ESP_OK) return false;
        }
        */
        //CSON();
        uint8_t mux = (positive_ch << 4) | (negative_ch & 0x0F);
        writeRegister(ADS1256_REG_MUX, mux);
        sync();
        wakeup();
        CSOFF();

        /*
        // Rilascio condizionale del bus
        if(need_acquire) {
            spi_device_release_bus(spi);
        }
        */

        return true;
    }

    bool set_gain(ads1256_gain_t gain) {
        if(!spi_initialized) {
            errorHandler.setError(ADS1256_Error::SPI_INIT_FAILED, 
                "SPI not initialized");
            return false;
        }
        if(gain > ADS1256_GAIN_64) {
            errorHandler.setError(ADS1256_Error::INVALID_GAIN, 
                "Gain not valid");
            return false;
        }
        /* 
        // Acquisizione condizionale del bus
        if(need_acquire) {
            esp_err_t ret = spi_device_acquire_bus(spi, portMAX_DELAY);
            if(ret != ESP_OK) return false;
        }
        */

        //uint8_t adcon = 0x20 | gain;  // Clock Out OFF + gain
        //writeRegister(ADS1256_REG_ADCON, adcon);
        //sync();
        //TSettleDelay();
        //CSON();
        uint8_t bytemask = B00000111;           // maschera per i 3 bit del gain
        uint8_t adcon = readRegister(ADS1256_REG_ADCON);    // legge il valore attuale
        uint8_t byte2send = (adcon & ~bytemask) | gain;  // mantiene gli altri bit e imposta il gain
        writeRegister(ADS1256_REG_ADCON, byte2send);
        TSettleDelay();
        /*
        // Rilascio condizionale del bus
        if(need_acquire) {
            spi_device_release_bus(spi);
        }
        */

        return true;
    }

    void print_spi_config() {
        Serial.println("\nConfigurazione SPI attuale:");
        
        spi_device_interface_config_t devcfg;
        memset(&devcfg, 0, sizeof(spi_device_interface_config_t));
        
        devcfg.mode = 1;                // CPOL=0, CPHA=1 
        devcfg.clock_speed_hz = 7800000;
        devcfg.spics_io_num = -1;       // CS manuale
        devcfg.queue_size = 1;
        devcfg.flags = SPI_DEVICE_NO_DUMMY;
        
        Serial.printf("Mode: %d (CPOL=%d, CPHA=%d)\n", 
                    devcfg.mode,
                    (devcfg.mode & 2) >> 1,
                    devcfg.mode & 1);
        Serial.printf("Clock: %d Hz\n", devcfg.clock_speed_hz);
        Serial.printf("CS: %d\n", devcfg.spics_io_num);
        Serial.printf("Flags: 0x%X\n", devcfg.flags);

        // Verifica tensioni di alimentazione e PDWN
        Serial.println("\nStati GPIO:");
        //Serial.printf("PDWN: %d\n", gpio_get_level((gpio_num_t)PDWN_PIN));  // Aggiungi il pin PDWN
        Serial.printf("CS: %d\n", gpio_get_level((gpio_num_t)ADS1256_PIN_CS));
        Serial.printf("MISO: %d\n", gpio_get_level((gpio_num_t)ADS1256_PIN_MISO));
        Serial.printf("MOSI: %d\n", gpio_get_level((gpio_num_t)ADS1256_PIN_MOSI));
        Serial.printf("SCLK: %d\n", gpio_get_level((gpio_num_t)ADS1256_PIN_SCK));
    }

   void test_chip_active() {
        Serial.println("\nTest chip active dopo power up");
        
        // Reset
        sendCommand(ADS1256_CMD_RESET);
        
        // Leggi STATUS
        uint8_t status = readRegister(ADS1256_REG_STATUS);
        Serial.printf("STATUS dopo power up: 0x%02X\n", status);
    }

    ADS1256Status getDeviceStatus() {
        ADS1256Status status = {0};
        
        // Verifica PWDN pin

        // Test chip attivo provando a leggere STATUS
        status.status = readRegister(ADS1256_REG_STATUS);
        status.chip_active = (status.status != 0xFF && status.status != 0x00);
        
        //if(status.chip_active) {
            // Se chip attivo, legge altri registri
            status.mux = readRegister(ADS1256_REG_MUX);
            status.adcon = readRegister(ADS1256_REG_ADCON);
            status.drate = readRegister(ADS1256_REG_DRATE);
            status.rdatac_active = (status.status & 0x80);
        //}
        
        return status;
    }

    void printDeviceStatus(const ADS1256Status& status) {
        Serial.printf("\nADS1256 Device Status:\n");
        Serial.printf("Chip Active: %s\n", status.chip_active ? "YES" : "NO");
        Serial.printf("RDATAC Mode: %s\n", status.rdatac_active ? "ON" : "OFF");
        
        Serial.printf("\nRegisters:\n");
        Serial.printf("STATUS: 0x%02X\n", status.status);
        Serial.printf("MUX: 0x%02X\n", status.mux);
        Serial.printf("ADCON: 0x%02X\n", status.adcon);
        Serial.printf("DRATE: 0x%02X\n", status.drate);
    }
    
private:
    static spi_device_handle_t spi;  // handle SPI statico
    //static spi_transaction_t tr;       // Variabile globale per le transazioni
    float emaFilteredValue;
    float emaAlpha = 0.1f;
    static bool isStreaming;  //  // flag statico per tracciare lo stato dello streaming
    static bool spi_initialized;  // flag statico
    static bool resetInProgress;  // flag statico
    uint8_t dma_buffer[32];
    // Test signal configuration
    bool testSignalEnabled;
    float testSignalTime = 0;
    float testFrequency = 1.0f;     // Hz
    float baseAmplitude = 1.25f;    // V (metà di 2.5V)
    float amFrequency = 0.1f;       // Hz
    const float vRef = 2.5f;        // Tensione di riferimento ADS1256
    float rampValue = 0.0f;  // Valore della rampa mantenuto tra i batch
    uint32_t offset = 0;
    ADS1256_ErrorHandler errorHandler;

    void wakeup(void) {
        sendCommand(ADS1256_CMD_WAKEUP);
    }

    void reset(void) {
        sendCommand(ADS1256_CMD_RESET);
    }

    bool sendCommand(uint8_t cmd) {
        if (!spi_initialized) return false;
        CSON();

        // Comandi che non richiedono DRDY
        if (cmd != ADS1256_CMD_SYNC && 
            cmd != ADS1256_CMD_RESET && 
            cmd != ADS1256_CMD_SDATAC) {
            waitDRDY();
        }
        
        spi_transaction_t t = {};
        t.length = 8;
        t.tx_buffer = &cmd;
        spi_device_transmit(spi, &t);
        
        if (cmd == ADS1256_CMD_RESET) {
            T11Delay();
        }
        T3Delay();
        CSOFF();
        return true;
    }

    static void sendCommand_static(uint8_t cmd) {
        if (!spi_initialized) return;
        
        // Comandi che non richiedono DRDY
        if (cmd != ADS1256_CMD_SYNC && 
            cmd != ADS1256_CMD_RESET && 
            cmd != ADS1256_CMD_SDATAC) {
            while(gpio_get_level((gpio_num_t)ADS1256_PIN_DRDY));
        }
        
        CSON_static();
        T2Delay();
        
        spi_transaction_t t = {};
        t.length = 8;
        t.tx_buffer = &cmd;
        spi_device_transmit(spi, &t);
        
        T6Delay();
        
        if (cmd == ADS1256_CMD_RESET) {
            T11Delay();
        }
        
        T3Delay();
        CSOFF_static();
    }

    uint8_t readRegister(uint8_t reg) {
        if(reg > ADS1256_REG_FSC2) return 0;
        if(!spi_initialized) return 0;

        bool was_streaming = isStreaming;
        if(was_streaming) {
            sendCommand(ADS1256_CMD_SDATAC);
            isStreaming = false;
        }

        waitDRDY();
        
        CSON();
        T2Delay();

        sendCommand(ADS1256_CMD_RDATA);
        T6Delay();

/*
        uint8_t cmd[2] = {ADS1256_CMD_RREG | reg, 0x00};
        spi_transaction_t t1 = {};
        t1.length = 16;
        t1.tx_buffer = cmd;
        spi_device_transmit(spi, &t1);
        T6Delay();

        uint8_t value;
        uint8_t dummy = 0;
        spi_transaction_t t2 = {};
        t2.length = 8;
        t2.rx_buffer = &value;
        t2.tx_buffer = &dummy;
        spi_device_transmit(spi, &t2);
*/
        uint8_t cmd[2] = {ADS1256_CMD_RREG | reg, 0x00};  // Comando di lettura registro
        uint8_t value = 0;

        spi_transaction_t t = {};
        t.length = 16;
        t.tx_buffer = cmd;
        t.rxlength = 8;
        t.rx_buffer = &value;

        esp_err_t ret = spi_device_transmit(spi, &t);

        T6Delay();
        T3Delay();
        CSOFF();
        
        if(was_streaming) {
            sendCommand(ADS1256_CMD_RDATAC);
            isStreaming = true;
        }

        return value;
    }

    bool writeRegister(uint8_t reg, uint8_t value) {
        if(reg > ADS1256_REG_FSC2) return false;
        if(!spi_initialized) return false;

        waitDRDY();
        
        CSON();
        T2Delay();

        uint8_t data[3] = {ADS1256_CMD_WREG | reg, 0x00, value};
        spi_transaction_t t = {};
        t.length = 24;
        t.tx_buffer = data;
        esp_err_t ret = spi_device_transmit(spi, &t);

        T6Delay();
        T3Delay();
        CSOFF();

        if(ret != ESP_OK) return false;

        uint8_t readback = readRegister(reg);// chiama ADS1256_CMD_RDATA che non interferisce con ADS1256_CMD_RDATAC
        
        return (readback == value);
    }

    inline void CSON() {
        gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 0);
        //digitalWrite(ADS1256_PIN_CS, LOW);  // CS attivo basso
        T2Delay();  // Delay dopo CS basso
    }

    inline void CSOFF() {
        T3Delay();  // Delay prima di CS alto
        //digitalWrite(ADS1256_PIN_CS, HIGH); // CS inattivo alto
        gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 1);
    }
/*
    void CSON(){
        gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 0);
    }

    void CSOFF(){
        gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 1);
    }
    */
/*
    // Timing functions for ADS1256

    // t1 - Tempo minimo CS basso (100ns)
    void T1Delay() {
        delayMicroseconds(1);  // Arduino non può fare delay di ns, usiamo 1us
    }

    // t2 - Setup CS al primo SCLK (50ns)
    void T2Delay() {
        delayMicroseconds(1);  // Minimo delay possibile
    }

    // t3 - Hold ultimo SCLK a CS alto (50ns)
    void T3Delay() {
        delayMicroseconds(1);  // Minimo delay possibile
    }

    // t4 - Disabilitazione CS (50ns)
    void T4Delay() {
        delayMicroseconds(1);  // Minimo delay possibile
    }

    // t5 - Setup DRDY dopo SYNC (24 * 1/7.68MHz = ~3.125us)
    void T5Delay() {
        delayMicroseconds(4);  // Arrotondato per sicurezza
    }

    // t6 - Setup DRDY dopo lettura (50ns)
    void T6Delay() {
        delayMicroseconds(1);  // Minimo delay possibile
    }

    // t11 - Power-on reset (25ms)
    void T11Delay() {
        delay(25);  // 25 millisecondi
    }

    // tSETTLE - Tempo di settling dopo cambio canale/guadagno
    // Dipende dal data rate. Per 30000 SPS:
    void TSettleDelay() {
        delayMicroseconds(400);  // Tipicamente 3-4 volte il periodo di conversione
    }
*/
    // Funzione helper per attendere che DRDY diventi basso
    void waitDRDY() {
        while(gpio_get_level((gpio_num_t)ADS1256_PIN_DRDY));
    }

    bool waitDRDY(uint32_t timeout) {
        uint32_t startTime = millis();        
        while(gpio_get_level((gpio_num_t)ADS1256_PIN_DRDY)) {
            if(millis() - startTime > timeout) {
                errorHandler.setError(ADS1256_Error::DRDY_TIMEOUT, 
                    "DRDY timeout waiting");
                return false; // Timeout della calibrazione
            }
            delay(1);
        }
        return true;
    }


    /*
    La funzione sync() è necessaria per l'ADS1256. Dal datasheet, il comando SYNC serve a:
    - Sincronizzare l'inizio di una conversione A/D
    - Far uscire il chip dallo standby o dalla modalità auto-calibrazione
    - Riavviare il modulatore sigma-delta

    È particolarmente importante dopo:
    - Cambio del canale di input (set_channel)
    - Modifica del gain (set_gain)
    - Modifica di altri parametri di configurazione

    La sequenza tipica è:
    - Scrittura registro 
    - SYNC
    - WAKEUP (opzionale)
    */
    void sync(void) {
        sendCommand(ADS1256_CMD_SYNC);
    }
};

// Definizione del flag statico
bool ADS1256_DMA::spi_initialized = false;
bool ADS1256_DMA::resetInProgress = false;
bool ADS1256_DMA::isStreaming = false;
spi_device_handle_t ADS1256_DMA::spi = nullptr;
//spi_transaction_t ADS1256_DMA::tr = {};  // Inizializzazione della tr statica
#endif

