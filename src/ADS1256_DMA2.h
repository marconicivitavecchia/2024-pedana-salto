#ifndef ADS1256_DMA_H
#define ADS1256_DMA_H

#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <Arduino.h>

// Pin definitions (unchanged)
#define ADS1256_PIN_CS   5
#define ADS1256_PIN_DRDY 4
#define ADS1256_PIN_MISO 19
#define ADS1256_PIN_MOSI 23
#define ADS1256_PIN_SCK  18

// Register definitions (unchanged)
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

// Commands (unchanged)
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

#define MAX_SAMPLES_PER_BATCH 310u

// Enums (unchanged)
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

struct BatchData {
    uint32_t timestamp;
    uint8_t count;
    uint8_t values[MAX_SAMPLES_PER_BATCH][3];
};

class ADS1256_DMA {
public:
    ADS1256_DMA() : emaFilteredValue(0.0f) {
        // Alloca buffer DMA allineato
        dma_buffer = (uint8_t*)heap_caps_malloc(DMA_BUFFER_SIZE, MALLOC_CAP_DMA);
        transactions = (spi_transaction_t*)heap_caps_malloc(
            sizeof(spi_transaction_t) * NUM_TRANSACTIONS, 
            MALLOC_CAP_DMA);
        
        // Setup SPI configuration
        spi_bus_config_t buscfg = {
            .mosi_io_num = ADS1256_PIN_MOSI,
            .miso_io_num = ADS1256_PIN_MISO,
            .sclk_io_num = ADS1256_PIN_SCK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = DMA_BUFFER_SIZE,
            .flags = SPICOMMON_BUSFLAG_MASTER,
            .intr_flags = 0
        };
        
        spi_device_interface_config_t devcfg = {
            .mode = 1,
            .clock_speed_hz = 7800000,
            .spics_io_num = -1,  // CS gestito manualmente
            .queue_size = NUM_TRANSACTIONS,
            .flags = SPI_DEVICE_NO_DUMMY
        };
        
        ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
        ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &devcfg, &spi));
        
        gpio_set_direction((gpio_num_t)ADS1256_PIN_CS, GPIO_MODE_OUTPUT);
        gpio_set_direction((gpio_num_t)ADS1256_PIN_DRDY, GPIO_MODE_INPUT);
        
        init_ads1256();
    }

    ~ADS1256_DMA() {
        if(dma_buffer) {
            heap_caps_free(dma_buffer);
        }
        if(transactions) {
            heap_caps_free(transactions);
        }
        if(spi) {
            spi_bus_remove_device(spi);
            spi_bus_free(HSPI_HOST);
        }
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
        float amp = baseAmplitude;

        const float voltsToAdc = 8388608.0f / vRef;
        
        rampValue += testFrequency * 0.005f;
        float volts = rampValue * vRef;

        if (volts < 0.0f) volts = 0.0f;
        if (rampValue > 1.0f) rampValue = 0.0f;

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

    void startStreaming() {
        if(isStreaming) return;

        gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 0);
        
        uint8_t read_cmd = ADS1256_CMD_RDATAC;
        spi_transaction_t trans_cmd;
        memset(&trans_cmd, 0, sizeof(spi_transaction_t));
        trans_cmd.length = 8;
        trans_cmd.tx_buffer = &read_cmd;
        spi_device_transmit(spi, &trans_cmd);
        
        delayMicroseconds(10);
        isStreaming = true;
    }

    void stopStreaming() {
        if(!isStreaming) return;

        uint8_t read_cmd = ADS1256_CMD_SDATAC;
        spi_transaction_t trans_cmd;
        memset(&trans_cmd, 0, sizeof(spi_transaction_t));
        trans_cmd.length = 8;
        trans_cmd.tx_buffer = &read_cmd;
        spi_device_transmit(spi, &trans_cmd);
        
        gpio_set_level((gpio_num_t)ADS1256_PIN_CS, 1);
        isStreaming = false;
    }

    void read_data_batch(BatchData& batch, uint16_t samplesPerBatch, uint16_t decimationFactor = 1) {
        if (testSignalEnabled) {
            generateTestSamples(batch, samplesPerBatch, decimationFactor);
            return;
        }
        
        if(!isStreaming) return;

        batch.count = 0;
        batch.timestamp = esp_timer_get_time();
        
        int32_t accumulator = 0;
        uint16_t decimationCount = 0;
        size_t dmaBufferIndex = 0;
        
        while(batch.count < samplesPerBatch) {
            accumulator = 0;
            decimationCount = 0;
            
            // Prepare DMA transactions for decimation
            while(decimationCount < decimationFactor) {
                while(gpio_get_level((gpio_num_t)ADS1256_PIN_DRDY));
                
                // Configure DMA transaction
                size_t trans_idx = decimationCount % NUM_TRANSACTIONS;
                memset(&transactions[trans_idx], 0, sizeof(spi_transaction_t));
                transactions[trans_idx].length = 24;
                transactions[trans_idx].rx_buffer = &dma_buffer[dmaBufferIndex];
                
                // Queue transaction
                ESP_ERROR_CHECK(spi_device_queue_trans(spi, &transactions[trans_idx], portMAX_DELAY));
                
                dmaBufferIndex = (dmaBufferIndex + 3) % DMA_BUFFER_SIZE;
                decimationCount++;
                
                // Collect results if pipeline is full or last sample
                if((decimationCount % NUM_TRANSACTIONS) == 0 || decimationCount == decimationFactor) {
                    size_t transactions_to_process = (decimationCount % NUM_TRANSACTIONS) == 0 ? 
                                                   NUM_TRANSACTIONS : 
                                                   decimationCount % NUM_TRANSACTIONS;
                    
                    for(size_t i = 0; i < transactions_to_process; i++) {
                        spi_transaction_t* rtrans;
                        ESP_ERROR_CHECK(spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY));
                        
                        // Process the received data
                        size_t buffer_idx = (dmaBufferIndex - (transactions_to_process - i) * 3 + DMA_BUFFER_SIZE) % DMA_BUFFER_SIZE;
                        int32_t value = (dma_buffer[buffer_idx] << 16) | 
                                      (dma_buffer[buffer_idx + 1] << 8) | 
                                      dma_buffer[buffer_idx + 2];
                        
                        if(value & 0x800000) {
                            value |= 0xFF000000;
                        }
                        
                        value += offset;
                        accumulator += value;
                    }
                }
            }
            
            // Calculate decimated value
            int32_t averaged_value = accumulator / decimationFactor;
            
            // Apply EMA filter
            emaFilteredValue = (float)averaged_value * emaAlpha + 
                             emaFilteredValue * (1.0f - emaAlpha);
            int32_t finalValue = (int32_t)emaFilteredValue;
            
            // Store in batch
            batch.values[batch.count][0] = (finalValue >> 16) & 0xFF;
            batch.values[batch.count][1] = (finalValue >> 8) & 0xFF;
            batch.values[batch.count][2] = finalValue & 0xFF;
            
            batch.count++;
        }
    }

    bool set_channel(ads1256_channels_t positive_ch, ads1256_channels_t negative_ch = ADS1256_AINCOM) {
        if (positive_ch > ADS1256_AIN7) return false;
        if (negative_ch > ADS1256_AINCOM) return false;

        uint8_t mux = (positive_ch << 4) | (negative_ch & 0x0F);
        write_reg(ADS1256_REG_MUX, mux);
        sync();
        return true;
    }

    bool set_gain(ads1256_gain_t gain) {
        if (gain > ADS1256_GAIN_64) return false;

        uint8_t adcon = read_reg(ADS1256_REG_ADCON);
        adcon &= 0xF8;
        adcon |= gain;
        write_reg(ADS1256_REG_ADCON, adcon);
        sync();
        return true;
    }

    bool set_single_channel(ads1256_channels_t channel) {
        return set_channel(channel, ADS1256_AINCOM);
    }

    void setEMAalfa(float alfa) {
        emaAlpha = alfa;
    }

private:
    static const size_t DMA_BUFFER_SIZE = 1024;  // Deve essere potenza di 2
    static const size_t NUM_TRANSACTIONS = 8;    // Pipeline di transazioni
    
    spi_device_handle_t spi;
    uint8_t* dma_buffer;
    spi_transaction_t* transactions;
    float emaFilteredValue;
    float emaAlpha = 0.1f;
    bool isStreaming = false;
    
    // Test signal configuration
    bool testSignalEnabled = false;
    float testSignalTime = 0;
    float testFrequency = 1.0f;
    float baseAmplitude = 1.25f;
    float amFrequency = 0.1f;
    const float vRef = 2.5f;
    float rampValue = 0.0f;
    uint32_t offset = 0;

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
        uint8_t cmd[3];
        cmd[0] = ADS1256_CMD_WREG | reg;
        cmd[1] = 0x00;
        cmd[2] = data;

        spi_device_acquire_bus(spi, portMAX_DELAY);
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
        uint8_t cmd[2];
        cmd[0] = ADS1256_CMD_RREG | reg;
        cmd[1] = 0x00;
        
        uint8_t data;
        
        spi_device_acquire_bus(spi, portMAX_DELAY);
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

#endif