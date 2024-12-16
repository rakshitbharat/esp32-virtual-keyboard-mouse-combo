#pragma once

// Device Settings
const char* DEVICE_NAME = "ESP32 HID";
const char* MANUFACTURER = "ESP32";
const uint8_t BATTERY_LEVEL = 100;
const uint8_t MAX_CONNECTIONS = 3;  // Maximum number of connections to remember

// BLE Security
const bool USE_SECURITY = true;
const bool REQUIRE_CONFIRMATION = false;
const bool SHOW_PAIRING_POPUP = true;

// Performance settings
const uint16_t CPU_FREQUENCY = 240;  // MHz
const uint32_t BAUD_RATE = 115200;

// HID report intervals (microseconds)
const uint32_t MIN_REPORT_INTERVAL = 8000;   // 8ms minimum between reports
const uint32_t KEY_REPEAT_DELAY = 500000;    // 500ms before key repeat starts
const uint32_t KEY_REPEAT_INTERVAL = 30000;  // 30ms between repeated keys
