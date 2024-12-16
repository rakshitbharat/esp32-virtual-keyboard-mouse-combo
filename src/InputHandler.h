#pragma once

#include <Arduino.h>
#include "BLEManager.h"
#include "config.h"
#include "CommandTypes.h"

class InputHandler {
public:
    InputHandler(BLEManager& bleManager, QueueHandle_t cmdQueue);
    
    void handleCommand(const String& command);
    void update(); // For key repeat and other periodic tasks
    
private:
    void handleKeyboardCommand(const String& command);
    void handleMouseCommand(const String& command);
    void handleModifiers(const String& modifiers);
    void handleSpecialKeys(uint8_t key);
    void queueCommand(const Command& cmd);
    
    BLEManager& bleManager;
    QueueHandle_t commandQueue;
    unsigned long lastReport;
    unsigned long lastKeyPress;
    bool keyRepeating;
    char repeatKey;
    
    // Mouse state
    int16_t lastX;
    int16_t lastY;
    bool isDragging;
};
