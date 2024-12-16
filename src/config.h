#pragma once

// Device Settings
const char* const DEVICE_NAME = "ESP32-HID";
const char* const MANUFACTURER = "ESP32";
const uint8_t BATTERY_LEVEL = 100;
const uint8_t MAX_CONNECTIONS = 1;  // Reduced to 1 for better stability

// BLE Security
const bool USE_SECURITY = true;
const bool REQUIRE_CONFIRMATION = false;
const bool SHOW_PAIRING_POPUP = true;

// Performance settings
const uint16_t CPU_FREQUENCY = 240;  // MHz
const uint32_t BAUD_RATE = 115200;  // Standard rate for better compatibility

// HID report intervals (microseconds)
const uint32_t MIN_REPORT_INTERVAL = 8000;   // 8ms - standard USB HID interval
const uint32_t KEY_REPEAT_DELAY = 500000;    // 500ms before key repeat starts
const uint32_t KEY_REPEAT_INTERVAL = 33000;  // ~30Hz repeat rate

// Task and Queue Configuration
const uint8_t CORE_TASK_HID = 1;      // HID on core 1 for better performance
const uint8_t CORE_TASK_MONITOR = 1;  // Monitor on same core as HID
const uint32_t STACK_SIZE = 2048;     // Reduced stack size
const uint8_t HID_QUEUE_SIZE = 32;    // Standard queue size
const uint32_t BLE_CONNECT_TIMEOUT = 5000;     // 5 seconds connection timeout
const uint32_t BLE_RECONNECT_INTERVAL = 1000;  // 1 second between reconnection attempts

// Battery monitoring
const uint8_t BATTERY_PIN = 34;        // ADC pin for battery monitoring
const float BATTERY_THRESHOLD = 3.3f;  // Battery voltage threshold
const uint32_t BATTERY_CHECK_INTERVAL = 60000;  // Check battery every minute

// Watchdog timeout
const uint8_t WDT_TIMEOUT_SECONDS = 5;
