#include <Arduino.h>
#include "BLEManager.h"
#include "InputHandler.h"
#include "config.h"
#include <esp_sleep.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <esp_coexist.h>
#include <esp_task_wdt.h>

// Global objects
BLEManager bleManager;
InputHandler* inputHandler = nullptr;
QueueHandle_t commandQueue = nullptr;
SemaphoreHandle_t bleMutex = nullptr;
TaskHandle_t monitorTaskHandle = nullptr;

// Battery monitoring constants
const int BATTERY_PIN = 34;  
const float BATTERY_THRESHOLD = 3.3;  
const int WDT_TIMEOUT_SECONDS = 5;  

void initPowerManagement() {
    setCpuFrequencyMhz(CPU_FREQUENCY);
    esp_wifi_deinit();
    esp_coex_preference_set(ESP_COEX_PREFER_BT);
}

void initWatchdog() {
    esp_task_wdt_init(WDT_TIMEOUT_SECONDS, true);
    esp_task_wdt_add(NULL);
}

void checkBattery() {
    static uint32_t lastCheck = 0;
    uint32_t now = millis();
    if (now - lastCheck >= BLE_RECONNECT_INTERVAL) {
        lastCheck = now;
        if (analogRead(BATTERY_PIN) * 3.3 / 4095.0 < BATTERY_THRESHOLD) {
            Serial.write('B'); 
        }
    }
}

void monitorTask(void* parameter) {
    for (;;) {
        checkBattery();
        if (bleManager.isConnected()) {
            Command cmd;
            if (xQueueReceive(commandQueue, &cmd, 0) == pdTRUE) {
                if (xSemaphoreTake(bleMutex, portMAX_DELAY)) {
                    bleManager.processCommand(cmd);
                    xSemaphoreGive(bleMutex);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(MIN_REPORT_INTERVAL / 1000));
    }
}

void setup() {
    Serial.begin(BAUD_RATE);
    Serial.write('S'); 
    
    bleMutex = xSemaphoreCreateMutex();
    commandQueue = xQueueCreate(HID_QUEUE_SIZE, sizeof(Command));
    
    if (!bleMutex || !commandQueue) {
        Serial.write('E'); 
        while(1) delay(1000);
    }
    
    initPowerManagement();
    initWatchdog();
    
    if (!bleManager.begin()) {
        Serial.write('F'); 
        while(1) delay(1000);
    }
    
    inputHandler = new InputHandler(bleManager, commandQueue);
    
    xTaskCreatePinnedToCore(
        monitorTask,
        "Monitor",
        STACK_SIZE,
        NULL,
        1,
        &monitorTaskHandle,
        CORE_TASK_MONITOR
    );

    Serial.write('R'); 
}

void loop() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (inputHandler) {
            inputHandler->handleCommand(cmd);
        }
    }
    esp_task_wdt_reset();
    delay(1);
}
