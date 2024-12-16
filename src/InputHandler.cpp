#include "InputHandler.h"
#include "BLEManager.h"
#include <Arduino.h>

InputHandler::InputHandler(BLEManager& bleManager)
    : bleManager(bleManager), lastReport(0), lastKeyPress(0),
      keyRepeating(false), repeatKey(0), lastX(0), lastY(0), isDragging(false) {}

void InputHandler::handleCommand(const String& command) {
    if (command.startsWith("key:") || command.startsWith("special:")) {
        handleKeyboardCommand(command);
    } else if (command.startsWith("move:") || command.startsWith("click:") || command.startsWith("scroll:")) {
        handleMouseCommand(command);
    }
}

void InputHandler::handleKeyboardCommand(const String& command) {
    Command cmd;
    
    if (command.startsWith("key:")) {
        cmd.type = Command::KEY_PRESS;
        cmd.data.key = command.substring(4).charAt(0);
        keyRepeating = true;
        repeatKey = cmd.data.key;
        lastKeyPress = millis();
    }
    else if (command.startsWith("special:")) {
        String special = command.substring(8);
        cmd.type = Command::KEY_SPECIAL;
        
        if (special == "enter") cmd.data.special_key = KEY_RETURN;
        else if (special == "backspace") cmd.data.special_key = KEY_BACKSPACE;
        else if (special == "space") {
            cmd.type = Command::KEY_PRESS;
            cmd.data.key = ' ';
        }
        else if (special == "tab") cmd.data.special_key = KEY_TAB;
        else if (special == "esc") cmd.data.special_key = KEY_ESC;
        
        keyRepeating = true;
        repeatKey = cmd.data.special_key;
        lastKeyPress = millis();
    }
    
    queueCommand(cmd);
}

void InputHandler::handleMouseCommand(const String& command) {
    Command cmd;
    
    if (command.startsWith("move:")) {
        int commaIndex = command.indexOf(',', 5);
        if (commaIndex != -1) {
            cmd.type = Command::MOUSE_MOVE;
            cmd.data.mouse.x = constrain(command.substring(5, commaIndex).toInt(), -127, 127);
            cmd.data.mouse.y = constrain(command.substring(commaIndex + 1).toInt(), -127, 127);
            cmd.data.mouse.scroll = 0;
            
            lastX = cmd.data.mouse.x;
            lastY = cmd.data.mouse.y;
        }
    }
    else if (command.startsWith("click:")) {
        cmd.type = Command::MOUSE_CLICK;
        String button = command.substring(6);
        
        if (button == "left") cmd.data.special_key = MOUSE_LEFT;
        else if (button == "right") cmd.data.special_key = MOUSE_RIGHT;
        else if (button == "middle") cmd.data.special_key = MOUSE_MIDDLE;
        
        if (button == "left" && !isDragging) {
            isDragging = true;
        } else if (button == "left" && isDragging) {
            isDragging = false;
        }
    }
    else if (command.startsWith("scroll:")) {
        cmd.type = Command::MOUSE_SCROLL;
        cmd.data.mouse.scroll = constrain(command.substring(7).toInt(), -127, 127);
        cmd.data.mouse.x = 0;
        cmd.data.mouse.y = 0;
    }
    
    queueCommand(cmd);
}

void InputHandler::queueCommand(const Command& cmd) {
    if (xQueueSend(commandQueue, &cmd, 0) != pdTRUE) {
        log_e("Command queue full!");
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
