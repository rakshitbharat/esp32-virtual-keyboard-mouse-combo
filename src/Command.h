#pragma once

#include <Arduino.h>

struct Command {
    enum Type {
        INVALID,
        KEY_PRESS,
        KEY_SPECIAL,
        MOUSE_MOVE,
        MOUSE_CLICK,
        MOUSE_SCROLL
    };

    struct MouseData {
        int8_t x;
        int8_t y;
        int8_t scroll;
    };

    union Data {
        char key;
        uint8_t special_key;
        MouseData mouse;
    };

    Type type;
    Data data;
};
