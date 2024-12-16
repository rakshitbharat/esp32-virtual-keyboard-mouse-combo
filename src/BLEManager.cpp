#include "BLEManager.h"

BLEManager::BLEManager() 
    : bleKeyboard(DEVICE_NAME, MANUFACTURER, BATTERY_LEVEL)
    , bleMouse(DEVICE_NAME, MANUFACTURER, BATTERY_LEVEL) {
}

void BLEManager::begin() {
    bleKeyboard.begin();
    bleMouse.begin();
}

void BLEManager::printDeviceInfo() {
    Serial.println("\nReady for commands:");
    Serial.println("Keyboard:");
    Serial.println("  k:a           - Type 'a'");
    Serial.println("  k:ctrl+c      - Press Ctrl+C");
    Serial.println("  k:shift+hello - Type 'HELLO'");
    Serial.println("  k:alt+tab     - Alt+Tab");
    Serial.println("  k:win+d       - Windows+D");
    Serial.println("\nMouse:");
    Serial.println("  m:move:x,y    - Move mouse");
    Serial.println("  m:left        - Left click");
    Serial.println("  m:right       - Right click");
    Serial.println("  m:scroll:n    - Scroll n units");
    Serial.println("  m:press:left  - Press left button");
    Serial.println("  m:release:left- Release left button");
}
