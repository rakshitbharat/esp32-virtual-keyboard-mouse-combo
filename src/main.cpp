#include <Arduino.h>
#include <BleKeyboard.h>
#include <BleMouse.h>
#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <esp_gap_ble_api.h>

// Device Settings
const char* DEVICE_NAME = "ESP32 HID";
const char* MANUFACTURER = "ESP32";
const uint8_t BATTERY_LEVEL = 100;
const uint8_t MAX_CONNECTIONS = 3;  // Maximum number of connections to remember

// BLE Security
const bool USE_SECURITY = true;
const bool REQUIRE_CONFIRMATION = false;
const bool SHOW_PAIRING_POPUP = true;

// Performance settings
const uint16_t CPU_FREQUENCY = 240;  // MHz
const uint32_t BAUD_RATE = 115200;

// HID report intervals (microseconds)
const uint32_t MIN_REPORT_INTERVAL = 8000;   // 8ms minimum between reports
const uint32_t KEY_REPEAT_DELAY = 500000;    // 500ms before key repeat starts
const uint32_t KEY_REPEAT_INTERVAL = 30000;  // 30ms between repeated keys

// Global variables
BleKeyboard bleKeyboard(DEVICE_NAME, MANUFACTURER, BATTERY_LEVEL);
BleMouse bleMouse(DEVICE_NAME, MANUFACTURER, BATTERY_LEVEL);

// Timing variables
unsigned long lastReport = 0;
unsigned long lastKeyPress = 0;
bool keyRepeating = false;
char repeatKey = 0;

// Function prototypes
void initBLE();
void handleKeyboard(const String& command);
void handleMouse(const String& command);
void printStatus();
void checkBattery();
void handleSpecialKeys(uint8_t key);
void handleModifiers(const String& modifiers);

void setup() {
    // Initialize Serial for debugging
    Serial.begin(BAUD_RATE);
    Serial.println("ESP32 BLE HID Device Starting...");
    
    // Set CPU frequency for optimal BLE performance
    if (setCpuFrequencyMhz(CPU_FREQUENCY)) {
        Serial.printf("CPU Frequency set to %dMHz\n", CPU_FREQUENCY);
    }
    
    // Initialize BLE with security
    initBLE();
    
    // Start HID services
    if (!bleKeyboard.begin()) {
        Serial.println("Failed to start keyboard service!");
        ESP.restart();
    }
    
    if (!bleMouse.begin()) {
        Serial.println("Failed to start mouse service!");
        ESP.restart();
    }
    
    // Print MAC address for pairing
    uint8_t* mac = esp_bt_dev_get_address();
    Serial.printf("Device MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    Serial.println("\nReady for commands:");
    Serial.println("Keyboard:");
    Serial.println("  k:a           - Type 'a'");
    Serial.println("  k:ctrl+c      - Press Ctrl+C");
    Serial.println("  k:shift+hello - Type 'HELLO'");
    Serial.println("  k:alt+tab     - Alt+Tab");
    Serial.println("  k:win+d       - Windows+D");
    Serial.println("\nMouse:");
    Serial.println("  m:move:x,y    - Move mouse");
    Serial.println("  m:left        - Left click");
    Serial.println("  m:right       - Right click");
    Serial.println("  m:scroll:n    - Scroll n units");
    Serial.println("  m:press:left  - Press left button");
    Serial.println("  m:release:left- Release left button");
}

void initBLE() {
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
        esp_bt_controller_disable();
        delay(100);
    }
    
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    
    if (USE_SECURITY) {
        // Set security parameters
        esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
        esp_ble_io_cap_t iocap = REQUIRE_CONFIRMATION ? 
                                ESP_IO_CAP_OUT : ESP_IO_CAP_NONE;
        uint8_t key_size = 16;
        uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
        uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
        
        esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, 
                                     &auth_req, sizeof(auth_req));
        esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, 
                                     &iocap, sizeof(iocap));
        esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, 
                                     &key_size, sizeof(key_size));
        esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, 
                                     &init_key, sizeof(init_key));
        esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, 
                                     &rsp_key, sizeof(rsp_key));
    }
}

void handleModifiers(const String& modifiers) {
    if (modifiers.indexOf("ctrl") != -1) bleKeyboard.press(KEY_LEFT_CTRL);
    if (modifiers.indexOf("shift") != -1) bleKeyboard.press(KEY_LEFT_SHIFT);
    if (modifiers.indexOf("alt") != -1) bleKeyboard.press(KEY_LEFT_ALT);
    if (modifiers.indexOf("win") != -1) bleKeyboard.press(KEY_LEFT_GUI);
}

void handleSpecialKeys(uint8_t key) {
    switch (key) {
        case KEY_RETURN: Serial.println("Pressed: ENTER"); break;
        case KEY_ESC: Serial.println("Pressed: ESC"); break;
        case KEY_BACKSPACE: Serial.println("Pressed: BACKSPACE"); break;
        case KEY_TAB: Serial.println("Pressed: TAB"); break;
        case KEY_DELETE: Serial.println("Pressed: DELETE"); break;
        case KEY_INSERT: Serial.println("Pressed: INSERT"); break;
        case KEY_HOME: Serial.println("Pressed: HOME"); break;
        case KEY_END: Serial.println("Pressed: END"); break;
        case KEY_PAGE_UP: Serial.println("Pressed: PAGE UP"); break;
        case KEY_PAGE_DOWN: Serial.println("Pressed: PAGE DOWN"); break;
        default: Serial.printf("Pressed: Special key 0x%02X\n", key);
    }
}

void handleKeyboard(const String& command) {
    if (!bleKeyboard.isConnected()) {
        Serial.println("Keyboard not connected!");
        return;
    }

    if (command.length() < 2) return;
    String cmd = command.substring(2); // Remove "k:"
    
    // Check for modifiers
    bool hasModifiers = cmd.indexOf('+') != -1;
    String modifiers = "";
    String keys = cmd;
    
    if (hasModifiers) {
        int plusPos = cmd.indexOf('+');
        modifiers = cmd.substring(0, plusPos);
        keys = cmd.substring(plusPos + 1);
    }
    
    // Handle different key combinations
    if (keys.length() == 1) {
        // Single character
        if (hasModifiers) {
            handleModifiers(modifiers);
            bleKeyboard.press(keys[0]);
            delay(10);
            bleKeyboard.releaseAll();
            Serial.printf("Pressed: %s+%c\n", modifiers.c_str(), keys[0]);
        } else {
            bleKeyboard.write(keys[0]);
            Serial.printf("Pressed: %c\n", keys[0]);
        }
    }
    else if (keys == "enter") bleKeyboard.write(KEY_RETURN);
    else if (keys == "space") bleKeyboard.write(' ');
    else if (keys == "tab") bleKeyboard.write(KEY_TAB);
    else if (keys == "esc") bleKeyboard.write(KEY_ESC);
    else if (keys == "backspace") bleKeyboard.write(KEY_BACKSPACE);
    else if (keys == "delete") bleKeyboard.write(KEY_DELETE);
    else if (keys == "insert") bleKeyboard.write(KEY_INSERT);
    else if (keys == "home") bleKeyboard.write(KEY_HOME);
    else if (keys == "end") bleKeyboard.write(KEY_END);
    else if (keys == "pageup") bleKeyboard.write(KEY_PAGE_UP);
    else if (keys == "pagedown") bleKeyboard.write(KEY_PAGE_DOWN);
    else if (keys.length() > 1) {
        // Type multiple characters
        if (hasModifiers) handleModifiers(modifiers);
        bleKeyboard.print(keys);
        if (hasModifiers) bleKeyboard.releaseAll();
        Serial.printf("Typed: %s\n", keys.c_str());
    }
}

void handleMouse(const String& command) {
    if (!bleMouse.isConnected()) {
        Serial.println("Mouse not connected!");
        return;
    }

    if (command.length() < 2) return;
    String cmd = command.substring(2); // Remove "m:"
    
    static int16_t lastX = 0, lastY = 0;
    static bool isDragging = false;
    
    if (cmd.startsWith("move:")) {
        int commaPos = cmd.indexOf(',', 5);
        if (commaPos > 5) {
            int16_t x = cmd.substring(5, commaPos).toInt();
            int16_t y = cmd.substring(commaPos + 1).toInt();
            
            // Constrain values to valid HID range
            x = constrain(x, -127, 127);
            y = constrain(y, -127, 127);
            
            // Store last position for relative movements
            lastX = x;
            lastY = y;
            
            // Add small delay between movements for better tracking
            unsigned long now = micros();
            if (now - lastReport >= MIN_REPORT_INTERVAL) {
                bleMouse.move(x, y);
                lastReport = now;
                Serial.printf("Mouse moved: x=%d, y=%d\n", x, y);
            }
        }
    }
    else if (cmd == "left") {
        bleMouse.click(MOUSE_LEFT);
        Serial.println("Left click");
    }
    else if (cmd == "right") {
        bleMouse.click(MOUSE_RIGHT);
        Serial.println("Right click");
    }
    else if (cmd == "middle") {
        bleMouse.click(MOUSE_MIDDLE);
        Serial.println("Middle click");
    }
    else if (cmd.startsWith("scroll:")) {
        int8_t scroll = constrain(cmd.substring(7).toInt(), -127, 127);
        bleMouse.move(0, 0, scroll);
        Serial.printf("Scrolled: %d\n", scroll);
    }
    else if (cmd == "press:left") {
        bleMouse.press(MOUSE_LEFT);
        isDragging = true;
        Serial.println("Left button pressed (drag started)");
    }
    else if (cmd == "release:left") {
        bleMouse.release(MOUSE_LEFT);
        isDragging = false;
        Serial.println("Left button released (drag ended)");
    }
    else if (cmd == "doubleclick") {
        bleMouse.click(MOUSE_LEFT);
        delay(50);
        bleMouse.click(MOUSE_LEFT);
        Serial.println("Double clicked");
    }
}

void checkBattery() {
    static unsigned long lastBatteryCheck = 0;
    const unsigned long BATTERY_CHECK_INTERVAL = 60000; // Check every minute
    
    if (millis() - lastBatteryCheck >= BATTERY_CHECK_INTERVAL) {
        // In a real device, you would read the actual battery level here
        lastBatteryCheck = millis();
    }
}

void printStatus() {
    static unsigned long lastStatus = 0;
    const unsigned long STATUS_INTERVAL = 5000; // Print status every 5 seconds
    
    if (millis() - lastStatus >= STATUS_INTERVAL) {
        lastStatus = millis();
        
        Serial.printf("Status - Keyboard: %s, Mouse: %s, Battery: %d%%\n",
            bleKeyboard.isConnected() ? "Connected" : "Disconnected",
            bleMouse.isConnected() ? "Connected" : "Disconnected",
            BATTERY_LEVEL
        );
    }
}

void loop() {
    // Regular status checks
    printStatus();
    checkBattery();
    
    // Process incoming commands
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command.startsWith("k:")) {
            handleKeyboard(command);
        }
        else if (command.startsWith("m:")) {
            handleMouse(command);
        }
        else {
            Serial.println("Invalid command. Use 'k:' for keyboard or 'm:' for mouse");
        }
    }
    
    // Handle key repeating if active
    if (keyRepeating && repeatKey != 0) {
        unsigned long now = micros();
        if (now - lastKeyPress >= KEY_REPEAT_INTERVAL) {
            bleKeyboard.write(repeatKey);
            lastKeyPress = now;
        }
    }
    
    // Small delay to prevent watchdog reset and reduce power consumption
    delay(1);
}
