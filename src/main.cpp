#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BleKeyboard.h>
#include <BleMouse.h>

// Device Name
#define DEVICE_NAME "ESP32-HID-Controller"

// BLE Service and Characteristic UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define KEYBOARD_CHAR_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define MOUSE_CHAR_UUID    "beb5483f-36e1-4688-b7f5-ea07361b26a8"

// Forward declarations of handler functions
void handleKeyboardCommand(String command);
void handleMouseCommand(String command);

BLEServer* pServer = nullptr;
BLECharacteristic* pKeyboardCharacteristic = nullptr;
BLECharacteristic* pMouseCharacteristic = nullptr;
BleKeyboard keyboard("ESP32 Keyboard");
BleMouse mouse("ESP32 Mouse");

bool deviceConnected = false;

// BLE Server Callbacks
class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("Device connected");
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("Device disconnected");
        // Restart advertising to allow new connections
        pServer->startAdvertising();
    }
};

// Keyboard Characteristic Callbacks
class KeyboardCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            // Process keyboard input from React Native app
            // Example: value format could be "key:a" or "special:enter"
            String command = String(value.c_str());
            handleKeyboardCommand(command);
        }
    }
};

// Mouse Characteristic Callbacks
class MouseCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            // Process mouse input from React Native app
            // Example: value format could be "move:x,y" or "click:left"
            String command = String(value.c_str());
            handleMouseCommand(command);
        }
    }
};

void handleKeyboardCommand(String command) {
    // Example command handling
    if (command.startsWith("key:")) {
        char key = command.substring(4).charAt(0);
        keyboard.print(key);
    } else if (command == "special:enter") {
        keyboard.write(KEY_RETURN);
    }
    // Add more keyboard commands as needed
}

void handleMouseCommand(String command) {
    // Example command handling
    if (command.startsWith("move:")) {
        int commaIndex = command.indexOf(',', 5);
        int x = command.substring(5, commaIndex).toInt();
        int y = command.substring(commaIndex + 1).toInt();
        mouse.move(x, y);
    } else if (command == "click:left") {
        mouse.click(MOUSE_LEFT);
    }
    // Add more mouse commands as needed
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE HID Controller...");

    // Initialize BLE Device
    BLEDevice::init(DEVICE_NAME);
    
    // Create BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    // Create BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create BLE Characteristics
    pKeyboardCharacteristic = pService->createCharacteristic(
        KEYBOARD_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pKeyboardCharacteristic->setCallbacks(new KeyboardCallbacks());

    pMouseCharacteristic = pService->createCharacteristic(
        MOUSE_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pMouseCharacteristic->setCallbacks(new MouseCallbacks());

    // Start the service
    pService->start();

    // Start advertising
    pServer->getAdvertising()->start();
    Serial.println("BLE HID Controller is ready!");

    // Initialize HID devices
    keyboard.begin();
    mouse.begin();
}

void loop() {
    // Main loop can be used for additional tasks if needed
    delay(10);
}
