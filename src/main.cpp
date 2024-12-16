#include <Arduino.h>
#include "config.h"
#include "BLEManager.h"
#include "BLEMonitor.h"
#include "InputHandler.h"
#include "CommandTypes.h"

// Global variables
BLEManager bleManager;
BLEMonitor* bleMonitor = nullptr;
InputHandler* inputHandler = nullptr;
QueueHandle_t commandQueue;
SemaphoreHandle_t bleMutex = nullptr;

// Status tracking
unsigned long lastStatusUpdate = 0;
unsigned long lastBatteryCheck = 0;

void checkBattery() {
    const unsigned long BATTERY_CHECK_INTERVAL = 60000; // Check every minute
    
    if (millis() - lastBatteryCheck >= BATTERY_CHECK_INTERVAL) {
        // In a real device, you would read the actual battery level here
        lastBatteryCheck = millis();
    }
}

void printStatus() {
    const unsigned long STATUS_INTERVAL = 5000; // Print status every 5 seconds
    
    if (millis() - lastStatusUpdate >= STATUS_INTERVAL) {
        lastStatusUpdate = millis();
        
        Serial.printf("Status - Keyboard: %s, Mouse: %s, Battery: %d%%\n",
            bleManager.getKeyboard()->isConnected() ? "Connected" : "Disconnected",
            bleManager.getMouse()->isConnected() ? "Connected" : "Disconnected",
            BATTERY_LEVEL
        );
    }
}

void setup() {
    // Initialize serial with high baud rate
    Serial.begin(BAUD_RATE);
    log_i("Starting BLE HID Controller...");
    
    // Create synchronization primitives
    bleMutex = xSemaphoreCreateMutex();
    commandQueue = xQueueCreate(HID_QUEUE_SIZE, sizeof(Command));
    
    if (!bleMutex || !commandQueue) {
        log_e("Failed to create queue or mutex!");
        while(1) delay(1000); // Fatal error
    }
    
    // Set CPU frequency for optimal BLE performance
    if (setCpuFrequencyMhz(CPU_FREQUENCY)) {
        log_i("CPU Frequency set to %dMHz", CPU_FREQUENCY);
    }
    
    // Initialize BLE
    bleManager.begin();
    
    // Create and start monitor
    bleMonitor = new BLEMonitor(bleManager);
    bleMonitor->begin();
    
    // Create input handler
    inputHandler = new InputHandler(bleManager);
    
    log_i("BLE HID Controller ready!");
}

void loop() {
    // Regular status checks
    printStatus();
    checkBattery();
    
    // Process incoming commands
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (inputHandler && xSemaphoreTake(bleMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            inputHandler->handleCommand(command);
            xSemaphoreGive(bleMutex);
        }
    }
    
    // Update input handler state
    if (inputHandler) {
        inputHandler->update();
    }
    
    // Small delay to prevent watchdog reset and reduce power consumption
    vTaskDelay(pdMS_TO_TICKS(1));
}
