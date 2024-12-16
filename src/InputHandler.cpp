#include "InputHandler.h"
#include "config.h"

InputHandler::InputHandler(BLEManager& bleManager, QueueHandle_t queue)
    : bleManager(bleManager), commandQueue(queue) {}

void InputHandler::handleCommand(const String& cmd) {
    Command command;
    command.type = parseCommandType(cmd);
    
    if (command.type == Command::INVALID) {
        return;
    }

    switch (command.type) {
        case Command::KEY_PRESS:
            command.data.key = cmd[4];
            break;
            
        case Command::KEY_SPECIAL:
            if (cmd == "special:enter") {
                command.data.special_key = KEY_RETURN;
            } else if (cmd == "special:backspace") {
                command.data.special_key = KEY_BACKSPACE;
            } else if (cmd == "special:space") {
                command.data.key = ' ';
            } else {
                return;
            }
            break;
            
        case Command::MOUSE_MOVE: {
            int commaIndex = cmd.indexOf(',', 5);
            if (commaIndex != -1) {
                command.data.mouse.x = constrain(cmd.substring(5, commaIndex).toInt(), -127, 127);
                command.data.mouse.y = constrain(cmd.substring(commaIndex + 1).toInt(), -127, 127);
                command.data.mouse.scroll = 0;
            } else {
                return;
            }
            break;
        }
            
        case Command::MOUSE_CLICK:
            if (cmd.substring(6) == "left") {
                command.data.special_key = MOUSE_LEFT;
            } else if (cmd.substring(6) == "right") {
                command.data.special_key = MOUSE_RIGHT;
            } else if (cmd.substring(6) == "middle") {
                command.data.special_key = MOUSE_MIDDLE;
            } else {
                return;
            }
            break;
            
        case Command::MOUSE_SCROLL:
            command.data.mouse.scroll = constrain(cmd.substring(7).toInt(), -127, 127);
            command.data.mouse.x = 0;
            command.data.mouse.y = 0;
            break;
            
        default:
            return;
    }
    
    xQueueSend(commandQueue, &command, 0);
}

Command::Type InputHandler::parseCommandType(const String& cmd) {
    if (cmd.length() < 2) return Command::INVALID;
    
    if (cmd.startsWith("key:")) return Command::KEY_PRESS;
    if (cmd.startsWith("special:")) return Command::KEY_SPECIAL;
    if (cmd.startsWith("move:")) return Command::MOUSE_MOVE;
    if (cmd.startsWith("click:")) return Command::MOUSE_CLICK;
    if (cmd.startsWith("scroll:")) return Command::MOUSE_SCROLL;
    return Command::INVALID;
}
