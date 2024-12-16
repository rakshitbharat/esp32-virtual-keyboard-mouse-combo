#include <Arduino.h>
#include <BleKeyboard.h>
#include <BleMouse.h>

// Basic device configuration
BleKeyboard bleKeyboard("ESP32 Keyboard");
BleMouse bleMouse("ESP32 Mouse");

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE Keyboard/Mouse...");
    
    // Set CPU frequency for better BLE performance
    setCpuFrequencyMhz(240);
    
    // Start BLE HID devices
    bleKeyboard.begin();
    bleMouse.begin();
    
    Serial.println("BLE Keyboard/Mouse Ready");
}

void processKeyboardCommand(const String& cmd) {
    if (!bleKeyboard.isConnected()) {
        Serial.println("Keyboard not connected");
        return;
    }

    if (cmd.startsWith("key:")) {
        char key = cmd.substring(4).charAt(0);
        bleKeyboard.write(key);
        Serial.printf("Key pressed: %c\n", key);
    }
    else if (cmd == "enter") {
        bleKeyboard.write(KEY_RETURN);
        Serial.println("Enter pressed");
    }
    else if (cmd == "space") {
        bleKeyboard.write(' ');
        Serial.println("Space pressed");
    }
    else if (cmd == "backspace") {
        bleKeyboard.write(KEY_BACKSPACE);
        Serial.println("Backspace pressed");
    }
    else if (cmd == "tab") {
        bleKeyboard.write(KEY_TAB);
        Serial.println("Tab pressed");
    }
}

void processMouseCommand(const String& cmd) {
    if (!bleMouse.isConnected()) {
        Serial.println("Mouse not connected");
        return;
    }

    if (cmd.startsWith("move:")) {
        int commaIndex = cmd.indexOf(',');
        if (commaIndex > 5) {
            int x = cmd.substring(5, commaIndex).toInt();
            int y = cmd.substring(commaIndex + 1).toInt();
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
        int scroll = cmd.substring(7).toInt();
        scroll = constrain(scroll, -127, 127);
        bleMouse.move(0, 0, scroll);
        Serial.printf("Scrolled: %d\n", scroll);
    }
}

void loop() {
    static unsigned long lastCheck = 0;
    const unsigned long checkInterval = 1000; // Check connection every second
    
    // Periodically check and report connection status
    unsigned long now = millis();
    if (now - lastCheck >= checkInterval) {
        lastCheck = now;
        
        static bool lastKeyboardState = false;
        static bool lastMouseState = false;
        
        bool currentKeyboardState = bleKeyboard.isConnected();
        bool currentMouseState = bleMouse.isConnected();
        
        if (currentKeyboardState != lastKeyboardState) {
            Serial.printf("Keyboard %s\n", currentKeyboardState ? "connected" : "disconnected");
            lastKeyboardState = currentKeyboardState;
        }
        
        if (currentMouseState != lastMouseState) {
            Serial.printf("Mouse %s\n", currentMouseState ? "connected" : "disconnected");
            lastMouseState = currentMouseState;
        }
    }
    
    // Process commands if available
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        // Split command into type and action
        int colonIndex = command.indexOf(':');
        if (colonIndex != -1) {
            String type = command.substring(0, colonIndex);
            String action = command.substring(colonIndex + 1);
            
            if (type == "kb") {
                processKeyboardCommand(action);
            }
            else if (type == "mouse") {
                processMouseCommand(action);
            }
        }
    }
    
    // Small delay to prevent watchdog reset and reduce power consumption
    delay(1);
}
