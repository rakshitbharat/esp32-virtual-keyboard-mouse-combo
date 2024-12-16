#pragma once

#include <BleKeyboard.h>
#include <BleMouse.h>
#include "Command.h"
#include "config.h"

class BLEManager {
public:
    BLEManager();
    bool begin();
    bool isConnected() const;
    void processCommand(const Command& cmd);

private:
    BleKeyboard keyboard;
    BleMouse mouse;
};
