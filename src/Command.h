#pragma once

#include <Arduino.h>

struct Command {
    enum Type {
        INVALID,
        KEY_PRESS,
        KEY_SPECIAL,
        KEY_MODIFIER,
        MOUSE_MOVE,
        MOUSE_CLICK,
        MOUSE_SCROLL,
        MOUSE_PRESS,
        MOUSE_RELEASE
    };

    struct MouseData {
        int16_t x;
        int16_t y;
        int8_t scroll;
    };

    struct ModifierData {
        uint8_t modifiers;
        uint8_t key;
    };

    union Data {
        char key;
        uint8_t special_key;
        MouseData mouse;
        ModifierData modifier;
    };

    Type type;
    Data data;
};
