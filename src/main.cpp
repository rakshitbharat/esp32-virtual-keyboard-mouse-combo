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

void InputHandler::handleCommand(String command) {
    Command cmd;
    
    if (command.startsWith("key:")) {
        // Check for modifiers (e.g., "key:ctrl+shift+a")
        String keyStr = command.substring(4);
        int plusPos = keyStr.lastIndexOf('+');
        
        if (plusPos != -1 && plusPos < keyStr.length() - 1) {
            cmd.type = Command::KEY_MODIFIER;
            cmd.data.mod_key.modifiers = 0;
            
            // Parse modifiers
            String modStr = keyStr.substring(0, plusPos);
            while (modStr.length() > 0) {
                int nextPlus = modStr.indexOf('+');
                String mod = (nextPlus == -1) ? modStr : modStr.substring(0, nextPlus);
                modStr = (nextPlus == -1) ? "" : modStr.substring(nextPlus + 1);
                
                if (mod.equalsIgnoreCase("ctrl")) cmd.data.mod_key.modifiers |= KEY_LEFT_CTRL;
                else if (mod.equalsIgnoreCase("shift")) cmd.data.mod_key.modifiers |= KEY_LEFT_SHIFT;
                else if (mod.equalsIgnoreCase("alt")) cmd.data.mod_key.modifiers |= KEY_LEFT_ALT;
                else if (mod.equalsIgnoreCase("gui")) cmd.data.mod_key.modifiers |= KEY_LEFT_GUI;
            }
            
            // Get the actual key
            cmd.data.mod_key.key = keyStr.charAt(keyStr.length() - 1);
        } else {
            cmd.type = Command::KEY_PRESS;
            cmd.data.key = keyStr.charAt(0);
        }
    }
    else if (command.startsWith("special:")) {
        String special = command.substring(8);
        cmd.type = Command::KEY_SPECIAL;
        
        if (special == "enter") cmd.data.special_key = KEY_RETURN;
        else if (special == "backspace") cmd.data.special_key = KEY_BACKSPACE;
        else if (special == "space") {
            cmd.type = Command::KEY_PRESS;
            cmd.data.key = ' ';
        }
        else if (special == "tab") cmd.data.special_key = KEY_TAB;
        else if (special == "esc") cmd.data.special_key = KEY_ESC;
        else if (special == "delete") cmd.data.special_key = KEY_DELETE;
        else if (special == "insert") cmd.data.special_key = KEY_INSERT;
        else if (special == "home") cmd.data.special_key = KEY_HOME;
        else if (special == "end") cmd.data.special_key = KEY_END;
        else if (special == "pageup") cmd.data.special_key = KEY_PAGE_UP;
        else if (special == "pagedown") cmd.data.special_key = KEY_PAGE_DOWN;
        else if (special == "right") cmd.data.special_key = KEY_RIGHT_ARROW;
        else if (special == "left") cmd.data.special_key = KEY_LEFT_ARROW;
        else if (special == "down") cmd.data.special_key = KEY_DOWN_ARROW;
        else if (special == "up") cmd.data.special_key = KEY_UP_ARROW;
    }
    else if (command.startsWith("move:") || command.startsWith("click:") || command.startsWith("press:") || command.startsWith("release:") || command.startsWith("scroll:")) {
        handleMouseCommand(command);
    }
    
    if (xQueueSend(commandQueue, &cmd, 0) != pdTRUE) {
        log_e("Command queue full!");
    }
}

void InputHandler::handleMouseCommand(String command) {
    Command cmd;
    
    if (command.startsWith("move:")) {
        int commaIndex = command.indexOf(',', 5);
        if (commaIndex != -1) {
            cmd.type = Command::MOUSE_MOVE;
            cmd.data.mouse.x = constrain(command.substring(5, commaIndex).toInt(), -127, 127);
            cmd.data.mouse.y = constrain(command.substring(commaIndex + 1).toInt(), -127, 127);
            cmd.data.mouse.scroll = 0;
        }
    }
    else if (command.startsWith("click:")) {
        cmd.type = Command::MOUSE_CLICK;
        String button = command.substring(6);
        
        if (button == "left") cmd.data.special_key = MOUSE_LEFT;
        else if (button == "right") cmd.data.special_key = MOUSE_RIGHT;
        else if (button == "middle") cmd.data.special_key = MOUSE_MIDDLE;
    }
    else if (command.startsWith("press:")) {
        cmd.type = Command::MOUSE_PRESS;
        String button = command.substring(6);
        
        if (button == "left") cmd.data.special_key = MOUSE_LEFT;
        else if (button == "right") cmd.data.special_key = MOUSE_RIGHT;
        else if (button == "middle") cmd.data.special_key = MOUSE_MIDDLE;
    }
    else if (command.startsWith("release:")) {
        cmd.type = Command::MOUSE_RELEASE;
        String button = command.substring(8);
        
        if (button == "left") cmd.data.special_key = MOUSE_LEFT;
        else if (button == "right") cmd.data.special_key = MOUSE_RIGHT;
        else if (button == "middle") cmd.data.special_key = MOUSE_MIDDLE;
    }
    else if (command.startsWith("scroll:")) {
        cmd.type = Command::MOUSE_SCROLL;
        cmd.data.mouse.scroll = constrain(command.substring(7).toInt(), -127, 127);
        cmd.data.mouse.x = 0;
        cmd.data.mouse.y = 0;
    }
    
    if (xQueueSend(commandQueue, &cmd, 0) != pdTRUE) {
        log_e("Command queue full!");
    }
}

void processCommand(const Command& cmd) {
    if (xSemaphoreTake(bleMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        switch (cmd.type) {
            case Command::KEY_PRESS:
                if (bleManager.getKeyboard()->isConnected()) {
                    bleManager.getKeyboard()->write(cmd.data.key);
                }
                break;
                
            case Command::KEY_SPECIAL:
                if (bleManager.getKeyboard()->isConnected()) {
                    bleManager.getKeyboard()->write(cmd.data.special_key);
                }
                break;
                
            case Command::KEY_MODIFIER:
                if (bleManager.getKeyboard()->isConnected()) {
                    if (cmd.data.mod_key.modifiers & KEY_LEFT_CTRL) bleManager.getKeyboard()->press(KEY_LEFT_CTRL);
                    if (cmd.data.mod_key.modifiers & KEY_LEFT_SHIFT) bleManager.getKeyboard()->press(KEY_LEFT_SHIFT);
                    if (cmd.data.mod_key.modifiers & KEY_LEFT_ALT) bleManager.getKeyboard()->press(KEY_LEFT_ALT);
                    if (cmd.data.mod_key.modifiers & KEY_LEFT_GUI) bleManager.getKeyboard()->press(KEY_LEFT_GUI);
                    
                    bleManager.getKeyboard()->press(cmd.data.mod_key.key);
                    delay(10);
                    bleManager.getKeyboard()->releaseAll();
                }
                break;
                
            case Command::MOUSE_MOVE:
                if (bleManager.getMouse()->isConnected()) {
                    bleManager.getMouse()->move(cmd.data.mouse.x, cmd.data.mouse.y);
                }
                break;
                
            case Command::MOUSE_CLICK:
                if (bleManager.getMouse()->isConnected()) {
                    bleManager.getMouse()->click(cmd.data.special_key);
                }
                break;
                
            case Command::MOUSE_PRESS:
                if (bleManager.getMouse()->isConnected()) {
                    bleManager.getMouse()->press(cmd.data.special_key);
                }
                break;
                
            case Command::MOUSE_RELEASE:
                if (bleManager.getMouse()->isConnected()) {
                    bleManager.getMouse()->release(cmd.data.special_key);
                }
                break;
                
            case Command::MOUSE_SCROLL:
                if (bleManager.getMouse()->isConnected()) {
                    bleManager.getMouse()->move(0, 0, cmd.data.mouse.scroll);
                }
                break;
        }
        xSemaphoreGive(bleMutex);
    }
}
