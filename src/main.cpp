#include <Arduino.h>
#include "config.h"
#include "BLEManager.h"
#include "InputHandler.h"

BLEManager bleManager;
InputHandler* inputHandler = nullptr;

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
    // Initialize Serial for debugging
    Serial.begin(BAUD_RATE);
    Serial.println("ESP32 BLE HID Device Starting...");
    
    // Set CPU frequency for optimal BLE performance
    if (setCpuFrequencyMhz(CPU_FREQUENCY)) {
        Serial.printf("CPU Frequency set to %dMHz\n", CPU_FREQUENCY);
    }
    
    // Initialize BLE
    bleManager.begin();
    
    // Wait for BLE to initialize
    delay(1000);
    
    // Check if BLE is working
    if (!bleManager.getKeyboard()->isConnected() && !bleManager.getMouse()->isConnected()) {
        Serial.println("BLE services not responding. Restarting...");
        delay(1000);
        ESP.restart();
    }
    
    // Print device info and commands
    bleManager.printDeviceInfo();
    
    // Create input handler
    inputHandler = new InputHandler(bleManager);
}

void loop() {
    // Regular status checks
    printStatus();
    checkBattery();
    
    // Process incoming commands
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (inputHandler) {
            inputHandler->handleCommand(command);
        }
    }
    
    // Update input handler state
    if (inputHandler) {
        inputHandler->update();
    }
    
    // Small delay to prevent watchdog reset and reduce power consumption
    delay(1);
}
