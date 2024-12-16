#pragma once

// Device Settings
const char* const DEVICE_NAME = "ESP32-HID";
const char* const MANUFACTURER = "ESP32";
const uint8_t BATTERY_LEVEL = 100;
const uint8_t MAX_CONNECTIONS = 3;  // Maximum number of connections to remember

// BLE Security
const bool USE_SECURITY = true;
const bool REQUIRE_CONFIRMATION = false;
const bool SHOW_PAIRING_POPUP = true;

// Performance settings
const uint16_t CPU_FREQUENCY = 240;  // MHz
const uint32_t BAUD_RATE = 921600;  // Increased for better performance

// HID report intervals (microseconds)
const uint32_t MIN_REPORT_INTERVAL = 8000;   // 8ms minimum between reports
const uint32_t KEY_REPEAT_DELAY = 500000;    // 500ms before key repeat starts
const uint32_t KEY_REPEAT_INTERVAL = 30000;  // 30ms between repeated keys

// Task and Queue Configuration
const uint8_t CORE_TASK_HID = 0;
const uint8_t CORE_TASK_MONITOR = 1;
const uint32_t STACK_SIZE = 4096;
const uint8_t HID_QUEUE_SIZE = 32;
const uint32_t BLE_CONNECT_TIMEOUT = 10000;  // 10 seconds timeout for connections
const uint32_t BLE_RECONNECT_INTERVAL = 5000;  // 5 seconds between reconnection attempts
