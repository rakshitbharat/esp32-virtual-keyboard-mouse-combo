#include <Arduino.h>
#include <BleKeyboard.h>
#include <BleMouse.h>

#define DEVICE_NAME "ESP32-HID-Controller"
#define CORE_TASK_HID 0  // Core 0 for HID tasks
#define CORE_TASK_MONITOR 1  // Core 1 for monitoring
#define STACK_SIZE 4096
#define HID_QUEUE_SIZE 32

// Queue handle for commands
QueueHandle_t commandQueue;

// Struct for commands to avoid string processing during runtime
struct Command {
    enum Type {
        KEY_PRESS,
        KEY_SPECIAL,
        MOUSE_MOVE,
        MOUSE_CLICK,
        MOUSE_SCROLL
    } type;
    union {
        char key;
        uint8_t special_key;
        struct {
            int16_t x;
            int16_t y;
            int8_t scroll;
        } mouse;
    } data;
};

BleKeyboard keyboard(DEVICE_NAME);
BleMouse mouse(DEVICE_NAME);

// Task handles
TaskHandle_t monitorTaskHandle = NULL;

void processCommand(const Command& cmd) {
    switch (cmd.type) {
        case Command::KEY_PRESS:
            if (keyboard.isConnected()) {
                keyboard.write(cmd.data.key);
            }
            break;
            
        case Command::KEY_SPECIAL:
            if (keyboard.isConnected()) {
                keyboard.write(cmd.data.special_key);
            }
            break;
            
        case Command::MOUSE_MOVE:
            if (mouse.isConnected()) {
                mouse.move(cmd.data.mouse.x, cmd.data.mouse.y);
            }
            break;
            
        case Command::MOUSE_CLICK:
            if (mouse.isConnected()) {
                mouse.click(cmd.data.special_key);
            }
            break;
            
        case Command::MOUSE_SCROLL:
            if (mouse.isConnected()) {
                mouse.move(0, 0, cmd.data.mouse.scroll);
            }
            break;
    }
}

void handleKeyboardCommand(String command) {
    Command cmd;
    
    if (command.startsWith("key:")) {
        cmd.type = Command::KEY_PRESS;
        cmd.data.key = command.substring(4).charAt(0);
    }
    else if (command == "special:enter") {
        cmd.type = Command::KEY_SPECIAL;
        cmd.data.special_key = KEY_RETURN;
    }
    else if (command == "special:backspace") {
        cmd.type = Command::KEY_SPECIAL;
        cmd.data.special_key = KEY_BACKSPACE;
    }
    else if (command == "special:space") {
        cmd.type = Command::KEY_PRESS;
        cmd.data.key = ' ';
    }
    
    xQueueSend(commandQueue, &cmd, 0); // Don't wait if queue is full
}

void handleMouseCommand(String command) {
    Command cmd;
    
    if (command.startsWith("move:")) {
        int commaIndex = command.indexOf(',', 5);
        if (commaIndex != -1) {
            cmd.type = Command::MOUSE_MOVE;
            cmd.data.mouse.x = command.substring(5, commaIndex).toInt();
            cmd.data.mouse.y = command.substring(commaIndex + 1).toInt();
            cmd.data.mouse.scroll = 0;
        }
    }
    else if (command.startsWith("click:")) {
        cmd.type = Command::MOUSE_CLICK;
        if (command == "click:left") {
            cmd.data.special_key = MOUSE_LEFT;
        }
        else if (command == "click:right") {
            cmd.data.special_key = MOUSE_RIGHT;
        }
        else if (command == "click:middle") {
            cmd.data.special_key = MOUSE_MIDDLE;
        }
    }
    else if (command.startsWith("scroll:")) {
        cmd.type = Command::MOUSE_SCROLL;
        cmd.data.mouse.scroll = command.substring(7).toInt();
        cmd.data.mouse.x = 0;
        cmd.data.mouse.y = 0;
    }
    
    xQueueSend(commandQueue, &cmd, 0); // Don't wait if queue is full
}

// Task for monitoring connection status
void monitorTask(void * parameter) {
    bool lastKeyboardStatus = false;
    bool lastMouseStatus = false;
    
    for(;;) {
        bool currentKeyboardStatus = keyboard.isConnected();
        bool currentMouseStatus = mouse.isConnected();

        if (currentKeyboardStatus != lastKeyboardStatus) {
            Serial.printf("Keyboard %s\n", currentKeyboardStatus ? "connected" : "disconnected");
            lastKeyboardStatus = currentKeyboardStatus;
        }

        if (currentMouseStatus != lastMouseStatus) {
            Serial.printf("Mouse %s\n", currentMouseStatus ? "connected" : "disconnected");
            lastMouseStatus = currentMouseStatus;
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Check status every 100ms
    }
}

void setup() {
    // Initialize serial at high baud rate
    Serial.begin(921600);
    
    // Create command queue
    commandQueue = xQueueCreate(HID_QUEUE_SIZE, sizeof(Command));
    
    // Initialize HID devices
    keyboard.begin();
    mouse.begin();
    
    // Create monitoring task on core 1
    xTaskCreatePinnedToCore(
        monitorTask,
        "MonitorTask",
        STACK_SIZE,
        NULL,
        1,
        &monitorTaskHandle,
        CORE_TASK_MONITOR
    );

    Serial.println("BLE HID Controller ready!");
}

void loop() {
    Command cmd;
    
    // Process commands from queue as fast as possible
    while (xQueueReceive(commandQueue, &cmd, 0) == pdTRUE) {
        processCommand(cmd);
    }
    
    // Minimal delay to prevent watchdog trigger
    taskYIELD();
}
