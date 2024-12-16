#pragma once

#include <Arduino.h>
#include "BLEManager.h"
#include "config.h"

class BLEMonitor {
public:
    BLEMonitor(BLEManager& manager);
    bool begin();

private:
    static void monitorTask(void* parameter);
    static BLEMonitor* instance;
    BLEManager& bleManager;
    TaskHandle_t taskHandle;
};
