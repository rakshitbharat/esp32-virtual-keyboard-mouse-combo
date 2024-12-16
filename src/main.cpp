#include <Arduino.h>
#include <BleKeyboard.h>
#include <BleMouse.h>

// Device Name
#define DEVICE_NAME "ESP32-HID-Controller"

BleKeyboard keyboard(DEVICE_NAME);
BleMouse mouse(DEVICE_NAME);

bool deviceConnected = false;

void handleKeyboardCommand(String command) {
    if (!keyboard.isConnected()) {
        Serial.println("Keyboard not connected");
        return;
    }

    if (command.startsWith("key:")) {
        char key = command.substring(4).charAt(0);
        keyboard.print(key);
        Serial.printf("Sending key: %c\n", key);
    } 
    else if (command == "special:enter") {
        keyboard.write(KEY_RETURN);
        Serial.println("Sending ENTER key");
    }
    else if (command == "special:backspace") {
        keyboard.write(KEY_BACKSPACE);
        Serial.println("Sending BACKSPACE key");
    }
    else if (command == "special:space") {
        keyboard.write(' ');
        Serial.println("Sending SPACE key");
    }
    // Add more special keys as needed
}

void handleMouseCommand(String command) {
    if (!mouse.isConnected()) {
        Serial.println("Mouse not connected");
        return;
    }

    if (command.startsWith("move:")) {
        int commaIndex = command.indexOf(',', 5);
        if (commaIndex != -1) {
            int x = command.substring(5, commaIndex).toInt();
            int y = command.substring(commaIndex + 1).toInt();
            mouse.move(x, y);
            Serial.printf("Moving mouse: x=%d, y=%d\n", x, y);
        }
    } 
    else if (command == "click:left") {
        mouse.click(MOUSE_LEFT);
        Serial.println("Left click");
    }
    else if (command == "click:right") {
        mouse.click(MOUSE_RIGHT);
        Serial.println("Right click");
    }
    else if (command == "click:middle") {
        mouse.click(MOUSE_MIDDLE);
        Serial.println("Middle click");
    }
    else if (command.startsWith("scroll:")) {
        int scroll = command.substring(7).toInt();
        mouse.move(0, 0, scroll);
        Serial.printf("Scrolling: %d\n", scroll);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE HID Controller...");

    keyboard.begin();
    mouse.begin();

    Serial.println("BLE HID Controller is ready!");
    Serial.println("Waiting for connections...");
}

void loop() {
    // Check connection status
    static bool lastKeyboardStatus = false;
    static bool lastMouseStatus = false;
    
    bool currentKeyboardStatus = keyboard.isConnected();
    bool currentMouseStatus = mouse.isConnected();

    // Report status changes
    if (currentKeyboardStatus != lastKeyboardStatus) {
        Serial.printf("Keyboard %s\n", currentKeyboardStatus ? "connected" : "disconnected");
        lastKeyboardStatus = currentKeyboardStatus;
    }

    if (currentMouseStatus != lastMouseStatus) {
        Serial.printf("Mouse %s\n", currentMouseStatus ? "connected" : "disconnected");
        lastMouseStatus = currentMouseStatus;
    }

    delay(10);
}
