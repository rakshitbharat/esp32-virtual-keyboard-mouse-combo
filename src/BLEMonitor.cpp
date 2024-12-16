#include "BLEMonitor.h"
#include <Arduino.h>

BLEMonitor* BLEMonitor::instance = nullptr;

BLEMonitor::BLEMonitor(BLEManager& manager) : bleManager(manager) {
    instance = this;
}

bool BLEMonitor::begin() {
    xTaskCreatePinnedToCore(
        monitorTask,
        "BLEMonitor",
        STACK_SIZE,
        this,
        1,
        &taskHandle,
        CORE_TASK_MONITOR
    );
    return true;
}

void BLEMonitor::monitorTask(void* parameter) {
    BLEMonitor* monitor = static_cast<BLEMonitor*>(parameter);
    
    while (true) {
        if (monitor->bleManager.isConnected()) {
            // Process any pending commands here
        }
        vTaskDelay(pdMS_TO_TICKS(MIN_REPORT_INTERVAL / 1000));
    }
}
