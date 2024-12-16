#include <Arduino.h>
#include "config.h"
#include "BLEManager.h"
#include "BLEMonitor.h"
#include "InputHandler.h"
#include "CommandTypes.h"
#include <esp_wifi.h>
#include <esp_bt.h>
#include <esp_coexist.h>
#include <esp_pm.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#include <esp_ota_ops.h>

// Global variables
BLEManager bleManager;
BLEMonitor* bleMonitor = nullptr;
InputHandler* inputHandler = nullptr;
QueueHandle_t commandQueue;
SemaphoreHandle_t bleMutex = nullptr;

// Status tracking
unsigned long lastStatusUpdate = 0;
unsigned long lastBatteryCheck = 0;

// Power management configuration
#define CONFIG_PM_ENABLE
#define DEFAULT_LIGHT_SLEEP_ENABLE true

// Battery monitoring
#define BATTERY_PIN 34  // ADC pin for battery monitoring
#define BATTERY_CHECK_INTERVAL 60000  // Check battery every minute
#define LOW_BATTERY_THRESHOLD 20      // Low battery warning at 20%
#define DEEP_SLEEP_THRESHOLD 10       // Enter deep sleep at 10%
#define DEEP_SLEEP_DURATION 1800000000 // 30 minutes in microseconds

uint8_t batteryLevel = 100;
bool lowBatteryWarning = false;

// Watchdog configuration
#define WDT_TIMEOUT 5  // 5 second watchdog timeout
#define MAX_CRASHES 3  // Maximum number of crashes before factory reset

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int crashCount = 0;

void initPowerManagement() {
    #ifdef CONFIG_PM_ENABLE
    // Configure dynamic frequency scaling:
    // Automatically scale CPU frequency between 80MHz and 240MHz
    esp_pm_config_esp32_t pm_config = {
        .max_freq_mhz = 240,
        .min_freq_mhz = 80,
        .light_sleep_enable = DEFAULT_LIGHT_SLEEP_ENABLE
    };
    esp_pm_configure(&pm_config);
    #endif
    
    // Disable WiFi
    esp_wifi_deinit();
    
    // Configure Bluetooth and WiFi coexistence
    esp_coex_preference_set(ESP_COEX_PREFER_BT);
}

void initWatchdog() {
    // Initialize watchdog
    esp_task_wdt_init(WDT_TIMEOUT, true); // true = panic on timeout
    esp_task_wdt_add(NULL); // Add current task to WDT
    
    // Check for crash recovery
    ++bootCount;
    if (esp_reset_reason() == ESP_RST_PANIC) {
        ++crashCount;
        log_w("System crashed! Crash count: %d", crashCount);
        
        if (crashCount >= MAX_CRASHES) {
            log_e("Too many crashes! Performing factory reset...");
            esp_partition_iterator_t pi = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
            if (pi != NULL) {
                const esp_partition_t* factory = esp_partition_get(pi);
                esp_partition_iterator_release(pi);
                esp_ota_set_boot_partition(factory);
                esp_restart();
            }
        }
    } else {
        crashCount = 0; // Reset crash count on normal boot
    }
    
    log_i("Boot count: %d", bootCount);
}

uint8_t getBatteryLevel() {
    // Read battery voltage from ADC
    uint32_t reading = analogRead(BATTERY_PIN);
    
    // Convert ADC reading to voltage (assuming 3.3V reference)
    float voltage = (reading * 3.3) / 4095.0;
    
    // Convert voltage to percentage (assuming 3.7V Li-Po battery)
    // 4.2V = 100%, 3.0V = 0%
    float percentage = ((voltage - 3.0) / 1.2) * 100.0;
    return constrain((int)percentage, 0, 100);
}

void checkBatteryStatus() {
    static unsigned long lastCheck = 0;
    unsigned long currentMillis = millis();
    
    if (currentMillis - lastCheck >= BATTERY_CHECK_INTERVAL) {
        batteryLevel = getBatteryLevel();
        
        // Update device battery level
        bleManager.getKeyboard()->setBatteryLevel(batteryLevel);
        bleManager.getMouse()->setBatteryLevel(batteryLevel);
        
        // Check for low battery
        if (batteryLevel <= LOW_BATTERY_THRESHOLD && !lowBatteryWarning) {
            lowBatteryWarning = true;
            log_w("Low battery warning: %d%%", batteryLevel);
        }
        
        // Check for critical battery level
        if (batteryLevel <= DEEP_SLEEP_THRESHOLD) {
            log_w("Critical battery level! Entering deep sleep...");
            esp_deep_sleep(DEEP_SLEEP_DURATION);
        }
        
        lastCheck = currentMillis;
    }
}

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
            batteryLevel
        );
    }
}

void setup() {
    // Initialize serial with high baud rate
    Serial.begin(921600);
    log_i("Starting BLE HID Controller...");
    
    // Initialize watchdog
    initWatchdog();
    
    // Initialize power management
    initPowerManagement();
    
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
    // Reset watchdog timer
    esp_task_wdt_reset();
    
    // Regular status checks
    printStatus();
    checkBatteryStatus();
    
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
    
    // If no activity for a while, enter light sleep
    static unsigned long lastActivity = 0;
    if (millis() - lastActivity > 300000) { // 5 minutes
        esp_task_wdt_reset(); // Reset watchdog before sleep
        esp_light_sleep_start();
        lastActivity = millis();
    }
    
    taskYIELD();
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
