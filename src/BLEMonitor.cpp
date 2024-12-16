#include "BLEMonitor.h"

BLEMonitor* BLEMonitor::instance = nullptr;

BLEMonitor::BLEMonitor(BLEManager& manager) 
    : bleManager(manager)
    , monitorTaskHandle(nullptr) {
    instance = this;
}

void BLEMonitor::begin() {
    xTaskCreatePinnedToCore(
        monitorTask,
        "MonitorTask",
        STACK_SIZE,
        this,
        1,
        &monitorTaskHandle,
        CORE_TASK_MONITOR
    );
}

void BLEMonitor::monitorTask(void* parameter) {
    BLEMonitor* monitor = (BLEMonitor*)parameter;
    bool lastKeyboardStatus = false;
    bool lastMouseStatus = false;
    uint32_t lastReconnectAttempt = 0;
    
    for(;;) {
        auto keyboard = monitor->bleManager.getKeyboard();
        auto mouse = monitor->bleManager.getMouse();
        
        bool currentKeyboardStatus = keyboard->isConnected();
        bool currentMouseStatus = mouse->isConnected();
        uint32_t currentMillis = millis();

        // Handle keyboard connection changes
        if (currentKeyboardStatus != lastKeyboardStatus) {
            if (currentKeyboardStatus) {
                log_i("Keyboard connected");
            } else {
                log_w("Keyboard disconnected");
                if (currentMillis - lastReconnectAttempt >= BLE_RECONNECT_INTERVAL) {
                    keyboard->end();
                    delay(100);
                    keyboard->begin();
                    lastReconnectAttempt = currentMillis;
                }
            }
            lastKeyboardStatus = currentKeyboardStatus;
        }

        // Handle mouse connection changes
        if (currentMouseStatus != lastMouseStatus) {
            if (currentMouseStatus) {
                log_i("Mouse connected");
            } else {
                log_w("Mouse disconnected");
                if (currentMillis - lastReconnectAttempt >= BLE_RECONNECT_INTERVAL) {
                    mouse->end();
                    delay(100);
                    mouse->begin();
                    lastReconnectAttempt = currentMillis;
                }
            }
            lastMouseStatus = currentMouseStatus;
        }

        // Check if BLE needs to be reinitialized
        if (!currentKeyboardStatus && !currentMouseStatus && 
            currentMillis - lastReconnectAttempt >= BLE_CONNECT_TIMEOUT) {
            log_w("BLE connection timeout, reinitializing...");
            monitor->bleManager.begin();
            lastReconnectAttempt = currentMillis;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
