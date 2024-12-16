#include "BLEManager.h"
#include <Arduino.h>

BLEManager::BLEManager() : keyboard(DEVICE_NAME), mouse(DEVICE_NAME) {}

bool BLEManager::begin() {
    keyboard.begin();
    mouse.begin();
    return true;
}

bool BLEManager::isConnected() const {
    BLEManager* nonConstThis = const_cast<BLEManager*>(this);
    return nonConstThis->keyboard.isConnected() && nonConstThis->mouse.isConnected();
}

void BLEManager::processCommand(const Command& cmd) {
    switch (cmd.type) {
        case Command::KEY_PRESS:
            if (keyboard.isConnected()) keyboard.write(cmd.data.key);
            break;

        case Command::KEY_SPECIAL:
            if (keyboard.isConnected()) keyboard.write(cmd.data.special_key);
            break;

        case Command::MOUSE_MOVE:
            if (mouse.isConnected()) mouse.move(cmd.data.mouse.x, cmd.data.mouse.y);
            break;

        case Command::MOUSE_CLICK:
            if (mouse.isConnected()) {
                switch(cmd.data.special_key) {
                    case MOUSE_LEFT:
                        mouse.click(MOUSE_LEFT);
                        break;
                    case MOUSE_RIGHT:
                        mouse.click(MOUSE_RIGHT);
                        break;
                    case MOUSE_MIDDLE:
                        mouse.click(MOUSE_MIDDLE);
                        break;
                }
            }
            break;

        case Command::MOUSE_SCROLL:
            if (mouse.isConnected()) mouse.move(0, 0, cmd.data.mouse.scroll);
            break;
    }
}
