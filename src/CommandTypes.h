#pragma once

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
