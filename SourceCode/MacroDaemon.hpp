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

#include "FIFOWatcher.hpp"
#include "FSWatcher.hpp"
#include "Packet.hpp"
#include "RemoteUDevice.hpp"
#include "UNIXSocket.hpp"
#include "XDG.hpp"

extern "C" {
#include <libnotify/notification.h>
}

/** kbdexCore
 *
 * Receive keyboard events from the kbdexKeyboardAgent and process them.
 *
 * TODO: улучшить документирующий комментарий.
 */
class MacroDaemon
{
private:
        UNIXServer         kbd_srv;
        UNIXSocket<Packet> *kbd_com = nullptr;
        RemoteUDevice      remote_udev;
        FSWatcher          fsw;
        XDG                xdg;

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

        /** Run a script match on an input event.
         *
         * @param sc Script to be executed.
         * @param ev Event to pass on to the script.
         * @param kbd_hid Human readable keyboard ID.
         * @return True if the key event should be repeated.
         */
        bool runScript(Lua::Script *sc, const struct input_event &ev, std::string kbd_hid);

        /** Get a connection to listen for keys on. */
        void getConnection();

        /**
         * Reload all scripts from their sources, this may be necessary
         * if an important configuration variable like the keymap is set.
         */
        void reloadAll();

        void startScriptWatcher();

public:
        MacroDaemon();
        ~MacroDaemon();

        /** Run the mainloop. */
        void run();
};
