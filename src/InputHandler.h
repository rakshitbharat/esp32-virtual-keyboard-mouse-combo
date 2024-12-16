#pragma once

#include "BLEManager.h"
#include "Command.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

class InputHandler {
public:
    InputHandler(BLEManager& bleManager, QueueHandle_t queue);
    void handleCommand(const String& cmd);

private:
    Command::Type parseCommandType(const String& cmd);
    BLEManager& bleManager;
    QueueHandle_t commandQueue;
};
