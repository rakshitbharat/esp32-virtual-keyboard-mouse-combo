#include "InputHandler.h"

InputHandler::InputHandler(BLEManager& manager) 
    : bleManager(manager)
    , lastReport(0)
    , lastKeyPress(0)
    , keyRepeating(false)
    , repeatKey(0)
    , lastX(0)
    , lastY(0)
    , isDragging(false) {
}

void InputHandler::handleCommand(const String& command) {
    if (command.startsWith("k:")) {
        handleKeyboard(command);
    }
    else if (command.startsWith("m:")) {
        handleMouse(command);
    }
    else {
        Serial.println("Invalid command. Use 'k:' for keyboard or 'm:' for mouse");
    }
}

void InputHandler::handleKeyboard(const String& command) {
    auto keyboard = bleManager.getKeyboard();
    if (!keyboard->isConnected()) {
        Serial.println("Keyboard not connected!");
        return;
    }

    if (command.length() < 2) return;
    String cmd = command.substring(2); // Remove "k:"
    
    // Check for modifiers
    bool hasModifiers = cmd.indexOf('+') != -1;
    String modifiers = "";
    String keys = cmd;
    
    if (hasModifiers) {
        int plusPos = cmd.indexOf('+');
        modifiers = cmd.substring(0, plusPos);
        keys = cmd.substring(plusPos + 1);
    }
    
    // Handle different key combinations
    if (keys.length() == 1) {
        if (hasModifiers) {
            handleModifiers(modifiers);
            keyboard->press(keys[0]);
            delay(10);
            keyboard->releaseAll();
            Serial.printf("Pressed: %s+%c\n", modifiers.c_str(), keys[0]);
        } else {
            keyboard->write(keys[0]);
            Serial.printf("Pressed: %c\n", keys[0]);
        }
    }
    else if (keys == "enter") keyboard->write(KEY_RETURN);
    else if (keys == "space") keyboard->write(' ');
    else if (keys == "tab") keyboard->write(KEY_TAB);
    else if (keys == "esc") keyboard->write(KEY_ESC);
    else if (keys == "backspace") keyboard->write(KEY_BACKSPACE);
    else if (keys == "delete") keyboard->write(KEY_DELETE);
    else if (keys == "insert") keyboard->write(KEY_INSERT);
    else if (keys == "home") keyboard->write(KEY_HOME);
    else if (keys == "end") keyboard->write(KEY_END);
    else if (keys == "pageup") keyboard->write(KEY_PAGE_UP);
    else if (keys == "pagedown") keyboard->write(KEY_PAGE_DOWN);
    else if (keys.length() > 1) {
        if (hasModifiers) handleModifiers(modifiers);
        keyboard->print(keys);
        if (hasModifiers) keyboard->releaseAll();
        Serial.printf("Typed: %s\n", keys.c_str());
    }
}

void InputHandler::handleMouse(const String& command) {
    auto mouse = bleManager.getMouse();
    if (!mouse->isConnected()) {
        Serial.println("Mouse not connected!");
        return;
    }

    if (command.length() < 2) return;
    String cmd = command.substring(2); // Remove "m:"
    
    if (cmd.startsWith("move:")) {
        int commaPos = cmd.indexOf(',', 5);
        if (commaPos > 5) {
            int16_t x = cmd.substring(5, commaPos).toInt();
            int16_t y = cmd.substring(commaPos + 1).toInt();
            
            // Constrain values to valid HID range
            x = constrain(x, -127, 127);
            y = constrain(y, -127, 127);
            
            // Store last position for relative movements
            lastX = x;
            lastY = y;
            
            // Add small delay between movements for better tracking
            unsigned long now = micros();
            if (now - lastReport >= MIN_REPORT_INTERVAL) {
                mouse->move(x, y);
                lastReport = now;
                Serial.printf("Mouse moved: x=%d, y=%d\n", x, y);
            }
        }
    }
    else if (cmd == "left") {
        mouse->click(MOUSE_LEFT);
        Serial.println("Left click");
    }
    else if (cmd == "right") {
        mouse->click(MOUSE_RIGHT);
        Serial.println("Right click");
    }
    else if (cmd == "middle") {
        mouse->click(MOUSE_MIDDLE);
        Serial.println("Middle click");
    }
    else if (cmd.startsWith("scroll:")) {
        int8_t scroll = constrain(cmd.substring(7).toInt(), -127, 127);
        mouse->move(0, 0, scroll);
        Serial.printf("Scrolled: %d\n", scroll);
    }
    else if (cmd == "press:left") {
        mouse->press(MOUSE_LEFT);
        isDragging = true;
        Serial.println("Left button pressed (drag started)");
    }
    else if (cmd == "release:left") {
        mouse->release(MOUSE_LEFT);
        isDragging = false;
        Serial.println("Left button released (drag ended)");
    }
    else if (cmd == "doubleclick") {
        mouse->click(MOUSE_LEFT);
        delay(50);
        mouse->click(MOUSE_LEFT);
        Serial.println("Double clicked");
    }
}

void InputHandler::handleModifiers(const String& modifiers) {
    auto keyboard = bleManager.getKeyboard();
    if (modifiers.indexOf("ctrl") != -1) keyboard->press(KEY_LEFT_CTRL);
    if (modifiers.indexOf("shift") != -1) keyboard->press(KEY_LEFT_SHIFT);
    if (modifiers.indexOf("alt") != -1) keyboard->press(KEY_LEFT_ALT);
    if (modifiers.indexOf("win") != -1) keyboard->press(KEY_LEFT_GUI);
}

void InputHandler::update() {
    // Handle key repeating if active
    if (keyRepeating && repeatKey != 0) {
        unsigned long now = micros();
        if (now - lastKeyPress >= KEY_REPEAT_INTERVAL) {
            bleManager.getKeyboard()->write(repeatKey);
            lastKeyPress = now;
        }
    }
}
