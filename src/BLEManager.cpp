#include "BLEManager.h"
#include <Arduino.h>

BLEManager::BLEManager() : keyboard("ESP32"), mouse("ESP32") {}

bool BLEManager::begin() {
    keyboard.begin();
    mouse.begin();
    Serial.println("BLE HID devices initialized");
    return true;
}

bool BLEManager::isConnected() const {
    // Remove const from method calls since the libraries don't support const
    BLEManager* nonConstThis = const_cast<BLEManager*>(this);
    return nonConstThis->keyboard.isConnected() && nonConstThis->mouse.isConnected();
}

void BLEManager::processCommand(const Command& cmd) {
    switch (cmd.type) {
        case Command::KEY_PRESS:
            if (keyboard.isConnected()) {
                keyboard.write(cmd.data.key);
            }
            break;
            
        case Command::KEY_SPECIAL:
            if (keyboard.isConnected()) {
                keyboard.write(cmd.data.special_key);
            }
            break;
            
        case Command::MOUSE_MOVE:
            if (mouse.isConnected()) {
                mouse.move(cmd.data.mouse.x, cmd.data.mouse.y);
            }
            break;
            
        case Command::MOUSE_CLICK:
            if (mouse.isConnected()) {
                mouse.click(cmd.data.special_key);
            }
            break;
            
        case Command::MOUSE_SCROLL:
            if (mouse.isConnected()) {
                mouse.move(0, 0, cmd.data.mouse.scroll);
            }
            break;
    }
}

void BLEManager::update() {
    static uint32_t lastReconnectAttempt = 0;
    const uint32_t RECONNECT_INTERVAL = 5000; // 5 seconds

    if (!isConnected()) {
        uint32_t now = millis();
        if (now - lastReconnectAttempt >= RECONNECT_INTERVAL) {
            lastReconnectAttempt = now;
            Serial.println("Attempting to reconnect BLE devices...");
            begin();
        }
    }
}
