#ifndef TASK_MONITOR_H
#define TASK_MONITOR_H

class TaskMonitor {
private:
    volatile uint32_t lastHeartbeatTime = 0;
    volatile uint32_t startTime = 0;
    const uint32_t timeoutMs;
    const uint32_t initialDelayMs = 8000;  // 5 secondi di delay iniziale
    TaskHandle_t taskHandle = nullptr;
    TaskFunction_t taskFunction;
    const char* taskName;
    uint32_t stackSize;
    UBaseType_t priority;
    void* taskParams;
    BaseType_t coreID;
    std::function<void()> cleanupCallback;  // Callback per la cleanup

public:
    TaskMonitor(const char* name, 
                uint32_t timeoutMillis,
                TaskFunction_t function,
                std::function<void()> cleanup,
                BaseType_t core = 0,
                uint32_t stack = 4096,
                UBaseType_t prio = 5,
                TaskMonitor* monitor = nullptr) : 
        timeoutMs(timeoutMillis),
        taskFunction(function),
        cleanupCallback(cleanup),
        taskName(name),
        stackSize(stack),
        priority(prio),
        taskParams(monitor),
        coreID(core) {
    }

    void setTaskParams(void* params) {
        taskParams = params;
    }

    void heartbeat() {
        lastHeartbeatTime = esp_timer_get_time() / 1000;
    }

    bool isAlive() {
        uint32_t currentTime = esp_timer_get_time() / 1000;
        
        if ((currentTime - startTime) < initialDelayMs) {
            return true;
        }
        
        return ((currentTime - lastHeartbeatTime) <= timeoutMs);
    }

    bool startTask() {
        if (taskHandle != nullptr) {
            return false;
        }
        
        BaseType_t result = xTaskCreatePinnedToCore(
            taskFunction,
            taskName,
            stackSize,
            taskParams,
            priority,
            &taskHandle,
            coreID
        );
        
        if (result == pdPASS) {
            uint32_t now = esp_timer_get_time() / 1000;
            lastHeartbeatTime = now;
            startTime = now;
            return true;
        }
        return false;
    }

    bool restartTask() {
        Serial.println("Tentativo di restart del task...");
        if (taskHandle != nullptr) {
            // Reset dello stato SPI prima di riavviare il task
            ADS1256_DMA::resetSPI();  // Reset stato SPI prima di killare il task
            vTaskDelete(taskHandle);
            taskHandle = nullptr;
            ADS1256_DMA::resetSPI();
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        Serial.println("Avvio nuovo task");
        return startTask();
    }

    TaskHandle_t getTaskHandle() {
        return taskHandle;
    }
};

#endif