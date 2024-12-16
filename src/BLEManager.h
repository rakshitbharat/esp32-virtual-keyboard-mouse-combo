#pragma once

#include <Arduino.h>
#include <BleKeyboard.h>
#include <BleMouse.h>
#include "config.h"
#include "CommandTypes.h"

class BLEManager {
public:
    BLEManager();
    bool begin();
    bool isConnected() const;
    void processCommand(const Command& cmd);
    void update();
    
    BleKeyboard* getKeyboard() { return &keyboard; }
    BleMouse* getMouse() { return &mouse; }
    
private:
    BleKeyboard keyboard;
    BleMouse mouse;
    bool initialized;
};
