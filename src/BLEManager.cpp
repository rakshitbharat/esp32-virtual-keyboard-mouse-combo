#include "BLEManager.h"
#include <esp_bt_main.h>
#include <esp_bt_device.h>

BLEManager::BLEManager()
    : keyboard(DEVICE_NAME, MANUFACTURER, 100)
    , mouse(DEVICE_NAME, MANUFACTURER, 100)
    , initialized(false) {
}

bool BLEManager::begin() {
    if (!initialized) {
        keyboard.begin();
        mouse.begin();
        initialized = true;
        log_i("BLE HID devices initialized");
    }
    return initialized;
}

bool BLEManager::isConnected() const {
    return keyboard.isConnected() && mouse.isConnected();
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
    // Handle any periodic BLE tasks here
    if (!isConnected()) {
        static uint32_t lastAttempt = 0;
        uint32_t now = millis();
        
        if (now - lastAttempt >= BLE_CONNECT_TIMEOUT) {
            log_i("Attempting to reconnect BLE devices...");
            begin();
            lastAttempt = now;
        }
    }
}
