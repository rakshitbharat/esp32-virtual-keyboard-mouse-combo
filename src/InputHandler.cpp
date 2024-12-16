#include "InputHandler.h"
#include <Arduino.h>

InputHandler::InputHandler(BLEManager& bleManager, QueueHandle_t cmdQueue)
    : bleManager(bleManager), commandQueue(cmdQueue), lastReport(0), lastKeyPress(0),
      keyRepeating(false), repeatKey(0), lastX(0), lastY(0), isDragging(false) {}

void InputHandler::handleCommand(const String& command) {
    if (command.startsWith("key:") || command.startsWith("special:")) {
        handleKeyboardCommand(command);
    } else if (command.startsWith("move:") || command.startsWith("click:") || 
               command.startsWith("scroll:")) {
        handleMouseCommand(command);
    }
}

void InputHandler::handleKeyboardCommand(const String& command) {
    Command cmd;
    bool validCommand = false;
    
    if (command.startsWith("key:")) {
        if (command.length() > 4) {
            cmd.type = Command::KEY_PRESS;
            cmd.data.key = command.substring(4).charAt(0);
            validCommand = true;
        }
    }
    else if (command == "special:enter") {
        cmd.type = Command::KEY_SPECIAL;
        cmd.data.special_key = KEY_RETURN;
        validCommand = true;
    }
    else if (command == "special:backspace") {
        cmd.type = Command::KEY_SPECIAL;
        cmd.data.special_key = KEY_BACKSPACE;
        validCommand = true;
    }
    else if (command == "special:space") {
        cmd.type = Command::KEY_PRESS;
        cmd.data.key = ' ';
        validCommand = true;
    }
    
    if (validCommand) {
        queueCommand(cmd);
    } else {
        Serial.println("Invalid keyboard command: " + command);
    }
}

void InputHandler::handleMouseCommand(const String& command) {
    Command cmd;
    bool validCommand = false;
    
    if (command.startsWith("move:")) {
        int commaIndex = command.indexOf(',', 5);
        if (commaIndex != -1) {
            cmd.type = Command::MOUSE_MOVE;
            cmd.data.mouse.x = constrain(command.substring(5, commaIndex).toInt(), -127, 127);
            cmd.data.mouse.y = constrain(command.substring(commaIndex + 1).toInt(), -127, 127);
            cmd.data.mouse.scroll = 0;
            validCommand = true;
        }
    }
    else if (command.startsWith("click:")) {
        String button = command.substring(6);
        cmd.type = Command::MOUSE_CLICK;
        
        if (button == "left") {
            cmd.data.special_key = MOUSE_LEFT;
            validCommand = true;
        }
        else if (button == "right") {
            cmd.data.special_key = MOUSE_RIGHT;
            validCommand = true;
        }
        else if (button == "middle") {
            cmd.data.special_key = MOUSE_MIDDLE;
            validCommand = true;
        }
    }
    else if (command.startsWith("scroll:")) {
        cmd.type = Command::MOUSE_SCROLL;
        cmd.data.mouse.scroll = constrain(command.substring(7).toInt(), -127, 127);
        cmd.data.mouse.x = 0;
        cmd.data.mouse.y = 0;
        validCommand = true;
    }
    
    if (validCommand) {
        queueCommand(cmd);
    } else {
        Serial.println("Invalid mouse command: " + command);
    }
}

void InputHandler::queueCommand(const Command& cmd) {
    if (xQueueSend(commandQueue, &cmd, 0) != pdTRUE) {
        Serial.println("Command queue full!");
    }
}

void InputHandler::update() {
    unsigned long currentTime = millis();
    
    // Handle key repeat
    if (keyRepeating && (currentTime - lastKeyPress >= KEY_REPEAT_INTERVAL)) {
        Command cmd;
        cmd.type = Command::KEY_PRESS;
        cmd.data.key = repeatKey;
        queueCommand(cmd);
        lastKeyPress = currentTime;
    }
    
    // Handle mouse dragging
    if (isDragging && (currentTime - lastReport >= MIN_REPORT_INTERVAL)) {
        Command cmd;
        cmd.type = Command::MOUSE_MOVE;
        cmd.data.mouse.x = lastX;
        cmd.data.mouse.y = lastY;
        cmd.data.mouse.scroll = 0;
        queueCommand(cmd);
        lastReport = currentTime;
    }
}
