#pragma once

#include <Arduino.h>
#include <BleKeyboard.h>
#include <BleMouse.h>
#include "config.h"

class BLEManager {
public:
    BLEManager();
    void begin();
    void printDeviceInfo();
    
    BleKeyboard* getKeyboard() { return &bleKeyboard; }
    BleMouse* getMouse() { return &bleMouse; }
    
private:
    BleKeyboard bleKeyboard;
    BleMouse bleMouse;
};
