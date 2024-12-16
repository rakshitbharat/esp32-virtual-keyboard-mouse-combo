#pragma once

#include <Arduino.h>
#include "BLEManager.h"
#include "config.h"

class BLEMonitor {
public:
    BLEMonitor(BLEManager& manager);
    void begin();
    static void monitorTask(void* parameter);
    
private:
    BLEManager& bleManager;
    TaskHandle_t monitorTaskHandle;
    static BLEMonitor* instance;
    
    void handleConnectionChanges();
    void attemptReconnection();
};
