#ifndef ADS1256_DMA_H
#define ADS1256_DMA_H

#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <Arduino.h>
#include "esp_log.h"

// Pin definitions
#define ADS1256_PIN_CS   5
#define ADS1256_PIN_DRDY 4
#define ADS1256_PIN_MISO 19
#define ADS1256_PIN_MOSI 23
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

class ADS1256_DMA {
public:
    ADS1256_DMA() : emaFilteredValue(0.0f) {
        Serial.println("Costruttore: inizio");
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
        
        spi_device_interface_config_t devcfg;
        memset(&devcfg, 0, sizeof(spi_device_interface_config_t));
        
        devcfg.command_bits = 0;
        devcfg.address_bits = 0;
        devcfg.dummy_bits = 0;
        devcfg.mode = 1;
        devcfg.duty_cycle_pos = 128;
        devcfg.cs_ena_pretrans = 0;
        devcfg.cs_ena_posttrans = 0;
        devcfg.clock_speed_hz = 7800000;
        devcfg.spics_io_num = -1;
        devcfg.flags = SPI_DEVICE_NO_DUMMY;
        devcfg.queue_size = 1;
        devcfg.pre_cb = NULL;
        devcfg.post_cb = NULL;

        
        Serial.printf("spi_initialized = %d\n", spi_initialized);
        Serial.printf("spi = %p\n", spi);
        if (!spi_initialized) {
            Serial.println("ADS1256_DMA: inizializzo SPI bus...");
            ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &buscfg, 1));
            Serial.println("ADS1256_DMA: SPI bus inizializzato");
            
            Serial.println("ADS1256_DMA: aggiungo device SPI...");
            ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &devcfg, &spi));
            Serial.println("ADS1256_DMA: device SPI aggiunto");
            spi_initialized = true;
        }        
        
        gpio_set_direction((gpio_num_t)ADS1256_PIN_CS, GPIO_MODE_OUTPUT);
        gpio_set_direction((gpio_num_t)ADS1256_PIN_DRDY, GPIO_MODE_INPUT);
        
        init_ads1256();
    }

    ~ADS1256_DMA() {
        if(spi) {
            spi_bus_remove_device(spi);
            spi_bus_free(HSPI_HOST);
        }
    }

    static void resetSPI() {
        // Previeni reset ricorsivi
        if (resetInProgress) {
            Serial.println("Reset già in corso, esco");
            return;
        }
        
        resetInProgress = true;
        Serial.println("resetSPI: inizio");
        
        // Prima ferma lo streaming
        if (isStreaming) {
            gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 1);
            isStreaming = false;
            // Dai tempo al dispositivo di notare il cambio di CS
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        
        if (spi_initialized && spi != nullptr) {
            // Resetta l'ADS1256
            uint8_t reset_cmd = ADS1256_CMD_RESET;
            spi_transaction_t trans = {};
            trans.length = 8;
            trans.tx_buffer = &reset_cmd;
            
            // Prova a inviare il comando di reset con timeout
            esp_err_t ret = spi_device_polling_transmit(spi, &trans);
            if (ret != ESP_OK) {
                Serial.printf("Errore nel reset del device: %s\n", esp_err_to_name(ret));
            }
            
            vTaskDelay(pdMS_TO_TICKS(50));
            
            // Rimuovi il device
            ret = spi_bus_remove_device(spi);
            if (ret != ESP_OK) {
                Serial.printf("Errore nella rimozione del device: %s\n", esp_err_to_name(ret));
            }
            spi = nullptr;
            
            vTaskDelay(pdMS_TO_TICKS(20));
            
            // Libera il bus
            ret = spi_bus_free(HSPI_HOST);
            if (ret != ESP_OK) {
                Serial.printf("Errore nella liberazione del bus: %s\n", esp_err_to_name(ret));
            }
        }
        
        // Reset pins
        gpio_reset_pin((gpio_num_t)ADS1256_PIN_MISO);
        gpio_reset_pin((gpio_num_t)ADS1256_PIN_MOSI);
        gpio_reset_pin((gpio_num_t)ADS1256_PIN_SCK);
        gpio_reset_pin((gpio_num_t)ADS1256_PIN_CS);
        gpio_reset_pin((gpio_num_t)ADS1256_PIN_DRDY);
        
        spi_initialized = false;
        resetInProgress = false;
        
        Serial.println("resetSPI: completato");
        
        // Yield per permettere ad altri task di eseguire
        vTaskDelay(1);
    }
    /* read_data_batch
    ---------------------------------------
        TEST SIGNAL GNERATION SECTION
    ---------------------------------------
    */
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
    int32_t read_data() {
        while(gpio_get_level((gpio_num_t)ADS1256_PIN_DRDY));
        
        // Aggiungiamo controllo iniziale
        if (!spi_initialized || spi == nullptr) return 0;
        
        esp_err_t ret = spi_device_acquire_bus(spi, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE("SPI", "Errore acquisizione bus: %s", esp_err_to_name(ret));
            return 0;
        }
        
        gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 0);
        
        uint8_t read_cmd = ADS1256_CMD_RDATA;
        spi_transaction_t trans_cmd;
        memset(&trans_cmd, 0, sizeof(spi_transaction_t));
        trans_cmd.length = 8;
        trans_cmd.tx_buffer = &read_cmd;
        spi_device_transmit(spi, &trans_cmd);
        
        spi_transaction_t trans_data;
        memset(&trans_data, 0, sizeof(spi_transaction_t));
        trans_data.length = 24;
        trans_data.rx_buffer = dma_buffer;
        spi_device_transmit(spi, &trans_data);
        
        gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 1);
        spi_device_release_bus(spi);
        
        int32_t value = (dma_buffer[0] << 16) | (dma_buffer[1] << 8) | dma_buffer[2];
        if(value & 0x800000) {
            value -= 0x1000000;
        }
        
        return value;
    }

    float apply_ema(int32_t value) {
        emaFilteredValue = emaAlpha * value + (1.0f - emaAlpha) * emaFilteredValue;
        return emaFilteredValue;
    }  

    void startStreaming() {
        if(isStreaming || !spi_initialized || spi == nullptr) return;

        esp_err_t ret = spi_device_acquire_bus(spi, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE("SPI", "Errore acquisizione bus: %s", esp_err_to_name(ret));
            return;
        }

        gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 0);
        
        uint8_t read_cmd = ADS1256_CMD_RDATAC;
        spi_transaction_t trans_cmd;
        memset(&trans_cmd, 0, sizeof(spi_transaction_t));
        trans_cmd.length = 8;
        trans_cmd.tx_buffer = &read_cmd;
        if(spi_device_transmit(spi, &trans_cmd) != ESP_OK) {
            gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 1);
            spi_device_release_bus(spi);
            return;
        }
        
        delayMicroseconds(10);
        isStreaming = true;
    }
/*
    static void stopStreaming() {
        if(!spi_initialized) return;  // Previene avvii multipli
        Serial.println("stopStreaming: inizio");
        if (isStreaming) {
            //Serial.println("stopStreaming: acquiring bus");
            //spi_device_acquire_bus(spi, portMAX_DELAY); // non funziona
            //esp_err_t ret = spi_device_acquire_bus(spi, portMAX_DELAY);pdMS_TO_TICKS(100)
            //esp_err_t ret = spi_device_acquire_bus(spi, portMAX_DELAY);
            //if (ret == ESP_OK) {
            if (true) {
                ESP_LOGI("SPI", "Bus SPI acquisito con successo.");

                Serial.println("stopStreaming: sending SDATAC");
                uint8_t read_cmd = ADS1256_CMD_SDATAC;
                spi_transaction_t trans_cmd;
                memset(&trans_cmd, 0, sizeof(spi_transaction_t));
                trans_cmd.length = 8;
                trans_cmd.tx_buffer = &read_cmd;
                spi_device_transmit(spi, &trans_cmd);

                delayMicroseconds(10);
                Serial.println("stopStreaming: setting CS high");
                gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 1);
                Serial.println("stopStreaming: releasing bus");
                
                esp_err_t ret = spi_device_acquire_bus(spi, portMAX_DELAY);
                if (ret == ESP_OK) {
                    // Esegui le operazioni sul bus SPI
                    spi_device_release_bus(spi);
                } else {
                    ESP_LOGE("SPI", "Errore nell'acquisizione del bus SPI: %s", esp_err_to_name(ret));
                }

                isStreaming = false;
                Serial.println("stopStreaming: completato");
            } else {
                ESP_LOGE("SPI", "Impossibile acquisire il bus SPI.");
            }      
        } else {
            Serial.println("stopStreaming: già fermo");
        }
    }
*/
    static void stopStreaming() {
        if(!spi_initialized || !isStreaming || spi == nullptr) return;

        // Disabilita gli interrupt se necessario
        //taskDISABLE_INTERRUPTS();
        
        // Force CS high per interrompere qualsiasi comunicazione
        gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 1);
        
        // Delay per assicurarsi che il chip noti il cambio di CS
        vTaskDelay(pdMS_TO_TICKS(10));
        
        // Prova a inviare SDATAC
        uint8_t read_cmd = ADS1256_CMD_SDATAC;
        spi_transaction_t trans_cmd = {};
        trans_cmd.length = 8;
        trans_cmd.tx_buffer = &read_cmd;
        spi_device_transmit(spi, &trans_cmd);
        
        // Aspetta che il comando faccia effetto
        vTaskDelay(pdMS_TO_TICKS(20));
        
        // Riabilita gli interrupt
        //taskENABLE_INTERRUPTS();
        
        isStreaming = false;
    }

    void read_data_batch(BatchData& batch, uint16_t samplesPerBatch, uint16_t decimationFactor = 1) {
        if (testSignalEnabled) {
            generateTestSamples(batch, samplesPerBatch, decimationFactor);
            return;
        }
        
        if(!isStreaming || !spi_initialized  || spi == nullptr) return;

        batch.count = 0;
        batch.timestamp = esp_timer_get_time();
        
        int32_t accumulator = 0;
        uint16_t decimationCount = 0;
        
        while(batch.count < samplesPerBatch) {
            accumulator = 0;
            decimationCount = 0;
            
            while(decimationCount < decimationFactor) {
                while(gpio_get_level((gpio_num_t)ADS1256_PIN_DRDY));
                
                uint32_t buffer = 0;
                spi_transaction_t trans_data = {};
                trans_data.length = 24;
                trans_data.rx_buffer = &buffer;
                spi_device_transmit(spi, &trans_data);

                if(buffer & 0x800000) {
                    buffer |= 0xFF000000;
                }

                buffer += offset;
                accumulator += buffer;
                decimationCount++;
            }
            
            int32_t averaged_value = accumulator / decimationFactor;
            emaFilteredValue = (float)averaged_value * emaAlpha + 
                            emaFilteredValue * (1.0f - emaAlpha);
            int32_t intValue = (int32_t)emaFilteredValue;
            
            batch.values[batch.count][0] = (intValue >> 16) & 0xFF;
            batch.values[batch.count][1] = (intValue >> 8) & 0xFF;
            batch.values[batch.count][2] = intValue & 0xFF;
            
            batch.count++;
        }
    }

    bool set_channel(ads1256_channels_t positive_ch, ads1256_channels_t negative_ch = ADS1256_AINCOM) {
        if(spi_initialized || spi != nullptr){
            if (positive_ch > ADS1256_AIN7) return false;
            if (negative_ch > ADS1256_AINCOM) return false;

            uint8_t mux = (positive_ch << 4) | (negative_ch & 0x0F);
            write_reg(ADS1256_REG_MUX, mux);
            sync();
            return true;
        }
        return false;
    }

    bool set_gain(ads1256_gain_t gain) {
        if(spi_initialized || spi != nullptr){
            if (gain > ADS1256_GAIN_64) return false;

            uint8_t adcon = read_reg(ADS1256_REG_ADCON);
            adcon &= 0xF8;
            adcon |= gain;
            write_reg(ADS1256_REG_ADCON, adcon);
            sync();
            return true;
        }
        return false;
    }

    bool set_single_channel(ads1256_channels_t channel) {
        if(spi_initialized || spi != nullptr){
            return set_channel(channel, ADS1256_AINCOM);
        }
        return false;
    }

    void setEMAalfa(float alfa){
        emaAlpha = alfa;
    }

private:
    static spi_device_handle_t spi;  // handle SPI statico
    uint8_t dma_buffer[32];
    float emaFilteredValue;
    float emaAlpha = 0.1f;
    static bool isStreaming;  //  // flag statico per tracciare lo stato dello streaming
    // Test signal configuration
    bool testSignalEnabled;
    float testSignalTime = 0;
    float testFrequency = 1.0f;     // Hz
    float baseAmplitude = 1.25f;    // V (metà di 2.5V)
    float amFrequency = 0.1f;       // Hz
    const float vRef = 2.5f;        // Tensione di riferimento ADS1256
    float rampValue = 0.0f;  // Valore della rampa mantenuto tra i batch
    uint32_t offset = 0;
    static bool spi_initialized;  // flag statico
    static bool resetInProgress;  // flag statico

    void init_ads1256() {
        gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 0);
        uint8_t reset_cmd = ADS1256_CMD_RESET;
        
        spi_transaction_t trans;
        memset(&trans, 0, sizeof(spi_transaction_t));
        trans.length = 8;
        trans.tx_buffer = &reset_cmd;
        spi_device_transmit(spi, &trans);
        
        gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 1);
        delay(1);
        
        write_reg(ADS1256_REG_STATUS, 0x03);  // Buffer OFF, no auto-cal
        write_reg(ADS1256_REG_MUX, 0x08);     // AIN0 vs AINCOM
        write_reg(ADS1256_REG_ADCON, 0x20);   // Clock Out OFF, Gain=1
        write_reg(ADS1256_REG_DRATE, 0xF0);   // 30000 SPS
        
        sync();
    }

    void sync(void) {
        gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 0);
        
        spi_transaction_t trans;
        memset(&trans, 0, sizeof(spi_transaction_t));
        trans.length = 8;
        uint8_t sync_cmd = ADS1256_CMD_SYNC;
        trans.tx_buffer = &sync_cmd;
        spi_device_transmit(spi, &trans);
        
        gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 1);
        delay(1);
    }

    void write_reg(uint8_t reg, uint8_t data) {
        if (!spi_initialized || spi == nullptr) return;

        uint8_t cmd[3];
        cmd[0] = ADS1256_CMD_WREG | reg;
        cmd[1] = 0x00;
        cmd[2] = data;

        esp_err_t ret = spi_device_acquire_bus(spi, portMAX_DELAY);
        if (ret != ESP_OK) return;

        gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 0);
        
        spi_transaction_t trans;
        memset(&trans, 0, sizeof(spi_transaction_t));
        trans.length = 24;
        trans.tx_buffer = cmd;
        spi_device_transmit(spi, &trans);
        
        gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 1);
        spi_device_release_bus(spi);
    }

    uint8_t read_reg(uint8_t reg) {
        if (!spi_initialized || spi == nullptr) return 0;

        uint8_t cmd[2];
        cmd[0] = ADS1256_CMD_RREG | reg;
        cmd[1] = 0x00;
        
        uint8_t data = 0;
        
        esp_err_t ret = spi_device_acquire_bus(spi, portMAX_DELAY);
        if (ret != ESP_OK) return 0;

        gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 0);
        
        spi_transaction_t trans_cmd;
        memset(&trans_cmd, 0, sizeof(spi_transaction_t));
        trans_cmd.length = 16;
        trans_cmd.tx_buffer = cmd;
        spi_device_transmit(spi, &trans_cmd);
        
        spi_transaction_t trans_data;
        memset(&trans_data, 0, sizeof(spi_transaction_t));
        trans_data.length = 8;
        trans_data.rx_buffer = &data;
        spi_device_transmit(spi, &trans_data);
        
        gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 1);
        spi_device_release_bus(spi);
        
        return data;
    }
};
// Definizione del flag statico
bool ADS1256_DMA::spi_initialized = false;
bool ADS1256_DMA::resetInProgress = false;
bool ADS1256_DMA::isStreaming = false;
spi_device_handle_t ADS1256_DMA::spi = nullptr;

#endif

