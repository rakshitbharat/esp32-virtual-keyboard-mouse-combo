#include <Arduino.h>
#include <BleKeyboard.h>
#include <BleMouse.h>

// Device Settings
const char* DEVICE_NAME = "ESP32 HID";
const char* MANUFACTURER = "ESP32";

// BLE HID Devices
BleKeyboard bleKeyboard(DEVICE_NAME, MANUFACTURER);
BleMouse bleMouse(DEVICE_NAME, MANUFACTURER);

// Function prototypes
void handleKeyboard(const String& command);
void handleMouse(const String& command);
void printStatus();

void setup() {
    // Initialize Serial for debugging
    Serial.begin(115200);
    Serial.println("ESP32 BLE HID Device Starting...");
    
    // Set CPU to maximum frequency for better BLE performance
    setCpuFrequencyMhz(240);
    
    // Start BLE HID services
    bleKeyboard.begin();
    bleMouse.begin();
    
    Serial.println("BLE HID services started");
    Serial.println("Ready to accept commands:");
    Serial.println("Keyboard: 'k:a' for letter, 'k:enter', 'k:space', etc.");
    Serial.println("Mouse: 'm:10,20' for move, 'm:left', 'm:right', 'm:scroll:5'");
}

void handleKeyboard(const String& command) {
    if (!bleKeyboard.isConnected()) {
        Serial.println("Keyboard not connected!");
        return;
    }

    if (command.length() < 2) return;

    String cmd = command.substring(2); // Remove "k:"
    
    if (cmd.length() == 1) {
        // Single character
        bleKeyboard.write(cmd[0]);
        Serial.printf("Pressed key: %c\n", cmd[0]);
    }
    else if (cmd == "enter") {
        bleKeyboard.write(KEY_RETURN);
        Serial.println("Pressed: ENTER");
    }
    else if (cmd == "space") {
        bleKeyboard.write(' ');
        Serial.println("Pressed: SPACE");
    }
    else if (cmd == "backspace") {
        bleKeyboard.write(KEY_BACKSPACE);
        Serial.println("Pressed: BACKSPACE");
    }
    else if (cmd == "tab") {
        bleKeyboard.write(KEY_TAB);
        Serial.println("Pressed: TAB");
    }
    else if (cmd == "esc") {
        bleKeyboard.write(KEY_ESC);
        Serial.println("Pressed: ESC");
    }
    else if (cmd.startsWith("ctrl+")) {
        if (cmd.length() > 5) {
            char key = cmd[5];
            bleKeyboard.press(KEY_LEFT_CTRL);
            bleKeyboard.press(key);
            delay(10);
            bleKeyboard.releaseAll();
            Serial.printf("Pressed: CTRL+%c\n", key);
        }
    }
}

void handleMouse(const String& command) {
    if (!bleMouse.isConnected()) {
        Serial.println("Mouse not connected!");
        return;
    }

    if (command.length() < 2) return;

    String cmd = command.substring(2); // Remove "m:"
    
    if (cmd.startsWith("move:")) {
        int commaPos = cmd.indexOf(',', 5);
        if (commaPos > 5) {
            int x = cmd.substring(5, commaPos).toInt();
            int y = cmd.substring(commaPos + 1).toInt();
            
            // Constrain values to valid HID range
            x = constrain(x, -127, 127);
            y = constrain(y, -127, 127);
            
            bleMouse.move(x, y);
            Serial.printf("Mouse moved: x=%d, y=%d\n", x, y);
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
        int scroll = constrain(cmd.substring(7).toInt(), -127, 127);
        bleMouse.move(0, 0, scroll);
        Serial.printf("Scrolled: %d\n", scroll);
    }
    else if (cmd == "press:left") {
        bleMouse.press(MOUSE_LEFT);
        Serial.println("Left button pressed");
    }
    else if (cmd == "release:left") {
        bleMouse.release(MOUSE_LEFT);
        Serial.println("Left button released");
    }
}

void printStatus() {
    static unsigned long lastStatus = 0;
    const unsigned long STATUS_INTERVAL = 5000; // Print status every 5 seconds
    
    unsigned long now = millis();
    if (now - lastStatus >= STATUS_INTERVAL) {
        lastStatus = now;
        
        Serial.printf("Status - Keyboard: %s, Mouse: %s\n",
            bleKeyboard.isConnected() ? "Connected" : "Disconnected",
            bleMouse.isConnected() ? "Connected" : "Disconnected"
        );
    }
}

void loop() {
    // Print connection status periodically
    printStatus();
    
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
            Serial.println("Invalid command format. Use 'k:' for keyboard or 'm:' for mouse");
        }
    }
    
    // Small delay to prevent watchdog reset and reduce power consumption
    delay(1);
}
