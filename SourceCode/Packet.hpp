/**
 * @file Packet.hpp
 *
 * @brief
 */

#pragma once

extern "C" {
#include <stdint.h>

#include <linux/input.h>
#include <linux/uinput.h>
}

enum class PacketType
{
        Command,
        KeyboardEvent
};

enum class Command
{
        PING,
        PONG,
}

/**
 * Структура, описывающая пакеты, которые используются для взаимодействия между kbdexKeyboardAgent
 * и kbdexCore.
 */
struct Packet
{
    PacketType type;

    union
    {
        // Команда от одного демона другому
        struct
        {
            Command command;
            uint8_t payload[512];  // опционально: данные, передаваемые вместе с командой.
        } cmd;

        // Событие клавиатуры. Копия структуры, которая использовалась в hawck.
        struct
        {
                // Whether this action signifies the end of a series of events
                uint8_t            done : 1;
                //The source keyboard for this event
                struct input_id    dev_id;
                // The event that was emitted, or should be emitted from the InputD UDevice
                struct input_event ev;
        } kbd_event;
    };

    Packet() = default;
};
