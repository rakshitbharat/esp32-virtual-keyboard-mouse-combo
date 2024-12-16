#pragma once

// Command structure for HID actions
struct Command {
    enum Type {
        KEY_PRESS,
        KEY_SPECIAL,
        KEY_MODIFIER,
        MOUSE_MOVE,
        MOUSE_CLICK,
        MOUSE_SCROLL,
        MOUSE_PRESS,
        MOUSE_RELEASE
    } type;
    
    union {
        char key;
        uint8_t special_key;
        struct {
            uint8_t modifiers;  // Combination of KEY_LEFT_CTRL, KEY_LEFT_SHIFT, etc.
            uint8_t key;
        } mod_key;
        struct {
            int16_t x;
            int16_t y;
            int8_t scroll;
        } mouse;
    } data;
};
