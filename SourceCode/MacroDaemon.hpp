/*
 * Исходный код в этом файле частично или полностью заимствован мной из проекта
 * hawck: https://github.com/snyball/hawck/tree/af318e24e63fb1ea87cb0080fe25ee293e5f2dd0
 */

/* =====================================================================================
 * Macro daemon.
 *
 * Copyright (C) 2018 Jonas Møller (no) <jonas.moeller2@protonmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * =====================================================================================
 */

/** @file MacroDaemon.hpp
 *
 * @brief Macro daemon.
 */

#pragma once

#include <chrono>
#include <string>
#include <vector>

extern "C" {
#include <libnotify/notification.h>
}

#include "FIFOWatcher.hpp"
#include "FSWatcher.hpp"
#include "Packet.hpp"
#include "RemoteUDevice.hpp"
#include "UNIXSocket.hpp"
#include "XDG.hpp"

#include "kbdex_config.h"


/** kbdexCore
 *
 * Receive keyboard events from the kbdexKeyboardAgent and process them.
 *
 * TODO: улучшить документирующий комментарий.
 */
class MacroDaemon
{
private:
        UNIXServer              kbd_srv;
        UNIXSocket<Packet>      *kbd_com = nullptr;
        RemoteUDevice           remote_udev;
        FSWatcher               fsw;
        XDG                     xdg;
        // TODO: необходимо учесть, что в три буфера ниже может прийти очень много событий, поэтому
        // нужно реализовать защиту от их чрезмерного использования.
        std::vector<Packet>             kbd_events_buffer;
        std::vector<Packet>             cmds_buffer;
        std::vector<std::string>        char_buffer;

        std::atomic<bool> notify_on_err;
        std::atomic<bool> stop_on_err;
        std::atomic<bool> eval_keydown;
        std::atomic<bool> eval_keyup;
        std::atomic<bool> eval_repeat;
        std::atomic<bool> disabled;

        std::mutex                           last_notification_mtx;
        std::tuple<std::string, std::string> last_notification;

        /** Display freedesktop DBus notification. */
        void notify(std::string title, std::string msg);

        /** Display freedesktop DBus notification. */
        void notify(std::string title, std::string msg, std::string icon, NotifyUrgency urgency);

        /** Get a connection to listen for keys on. */
        void getConnection();

        /**
         * Reload all scripts from their sources, this may be necessary
         * if an important configuration variable like the keymap is set.
         */
        void reloadAll();

        void startScriptWatcher();

        void processKbdEvent(const Packet& ev);

        void processCmd(const Packet& ev);

        std::string getCurrentKeyboardLayoutGnome() const;

        std::string getCurrentKeyboardLayoutKDE() const;

        void processBuffers();

public:
        MacroDaemon();
        ~MacroDaemon();

        /** Run the mainloop. */
        void run();
};
