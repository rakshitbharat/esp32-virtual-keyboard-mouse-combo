#include <Arduino.h>
#include <BleKeyboard.h>
#include <BleMouse.h>
#include <esp_bt_main.h>
#include <esp_bt_device.h>

// Device configuration
#define DEVICE_NAME "ESP32-HID"
#define MANUFACTURER "ESP32"
#define BLE_RETRY_COUNT 3

// Task and queue configuration
#define CORE_TASK_HID 0
#define CORE_TASK_MONITOR 1
#define STACK_SIZE 4096
#define HID_QUEUE_SIZE 32
#define BLE_CONNECT_TIMEOUT 10000 // 10 seconds timeout for connections

// Command structure for HID actions
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

// Global variables
QueueHandle_t commandQueue;
TaskHandle_t monitorTaskHandle = NULL;
SemaphoreHandle_t bleMutex = NULL;
volatile bool bleEnabled = false;

// BLE devices with battery level and connection timeout
BleKeyboard keyboard(DEVICE_NAME, MANUFACTURER, 100);
BleMouse mouse(DEVICE_NAME, MANUFACTURER, 100);

// Function declarations
void initBLE();
void processCommand(const Command& cmd);
void monitorTask(void* parameter);

// Initialize BLE and set security
void initBLE() {
    if (!bleEnabled) {
        // Initialize BLE
        if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
            esp_bt_controller_disable();
            delay(100);
        }
        
        esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        esp_bt_controller_init(&bt_cfg);
        esp_bt_controller_enable(ESP_BT_MODE_BLE);
        
        // Set BLE device properties
        esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
        esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
        uint8_t key_size = 16;
        uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
        uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
        
        esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(auth_req));
        esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(iocap));
        esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(key_size));
        esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(init_key));
        esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(rsp_key));
        
        bleEnabled = true;
    }
}

// Process HID commands with error handling
void processCommand(const Command& cmd) {
    if (xSemaphoreTake(bleMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
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
        xSemaphoreGive(bleMutex);
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
    
    if (xQueueSend(commandQueue, &cmd, 0) != pdTRUE) {
        log_e("Command queue full!");
    }
}

void handleMouseCommand(String command) {
    Command cmd;
    
    if (command.startsWith("move:")) {
        int commaIndex = command.indexOf(',', 5);
        if (commaIndex != -1) {
            cmd.type = Command::MOUSE_MOVE;
            cmd.data.mouse.x = constrain(command.substring(5, commaIndex).toInt(), -127, 127);
            cmd.data.mouse.y = constrain(command.substring(commaIndex + 1).toInt(), -127, 127);
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
        cmd.data.mouse.scroll = constrain(command.substring(7).toInt(), -127, 127);
        cmd.data.mouse.x = 0;
        cmd.data.mouse.y = 0;
    }
    
    if (xQueueSend(commandQueue, &cmd, 0) != pdTRUE) {
        log_e("Command queue full!");
    }
}

// Monitor task with reconnection handling
void monitorTask(void* parameter) {
    bool lastKeyboardStatus = false;
    bool lastMouseStatus = false;
    uint32_t lastReconnectAttempt = 0;
    const uint32_t reconnectInterval = 5000; // 5 seconds between reconnection attempts
    
    for(;;) {
        bool currentKeyboardStatus = keyboard.isConnected();
        bool currentMouseStatus = mouse.isConnected();
        uint32_t currentMillis = millis();

        // Handle keyboard connection changes
        if (currentKeyboardStatus != lastKeyboardStatus) {
            if (currentKeyboardStatus) {
                log_i("Keyboard connected");
            } else {
                log_w("Keyboard disconnected");
                if (currentMillis - lastReconnectAttempt >= reconnectInterval) {
                    keyboard.end();
                    delay(100);
                    keyboard.begin();
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
                if (currentMillis - lastReconnectAttempt >= reconnectInterval) {
                    mouse.end();
                    delay(100);
                    mouse.begin();
                    lastReconnectAttempt = currentMillis;
                }
            }
            lastMouseStatus = currentMouseStatus;
        }

        // Check if BLE needs to be reinitialized
        if (!currentKeyboardStatus && !currentMouseStatus && 
            currentMillis - lastReconnectAttempt >= BLE_CONNECT_TIMEOUT) {
            log_w("BLE connection timeout, reinitializing...");
            bleEnabled = false;
            initBLE();
            lastReconnectAttempt = currentMillis;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void setup() {
    // Initialize serial with high baud rate
    Serial.begin(921600);
    log_i("Starting BLE HID Controller...");
    
    // Create synchronization primitives
    bleMutex = xSemaphoreCreateMutex();
    commandQueue = xQueueCreate(HID_QUEUE_SIZE, sizeof(Command));
    
    if (!bleMutex || !commandQueue) {
        log_e("Failed to create queue or mutex!");
        while(1) delay(1000); // Fatal error
    }
    
    // Initialize BLE stack
    initBLE();
    
    // Start HID devices
    keyboard.begin();
    mouse.begin();
    
    // Create monitoring task
    xTaskCreatePinnedToCore(
        monitorTask,
        "MonitorTask",
        STACK_SIZE,
        NULL,
        1,
        &monitorTaskHandle,
        CORE_TASK_MONITOR
    );

    log_i("BLE HID Controller ready!");
}

void loop() {
    Command cmd;
    
    // Process commands from queue
    while (xQueueReceive(commandQueue, &cmd, 0) == pdTRUE) {
        processCommand(cmd);
    }
    
    taskYIELD();
}
