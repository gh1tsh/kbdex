/* =============================================================================
 *
 * Исходный код в этом файле частично или полностью заимствован мной из проекта
 * hawck: https://github.com/snyball/hawck/tree/af318e24e63fb1ea87cb0080fe25ee293e5f2dd0
 *
 * BSD 2-Clause License
 *
 * Copyright (c) 2019, Jonas Møller
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * =============================================================================
 */

#pragma once

#include <mutex>
#include <regex>
#include <vector>

#include "Keyboard.hpp"

extern "C" {
#include <syslog.h>
}

class KBDUnlock
{
        std::vector<Keyboard *> kbds;

public:
        inline KBDUnlock(std::vector<Keyboard *> &kbds) : kbds(kbds) {}

        ~KBDUnlock();
};

class KBDManager
{
private:
        /** Watcher for /dev/input/ hotplug */
        FSWatcher               input_fsw;
        /** All keyboards. */
        std::vector<Keyboard *> kbds;
        std::mutex              kbds_mtx;
        /** Keyboards available for listening. */
        std::vector<Keyboard *> available_kbds;
        std::mutex              available_kbds_mtx;
        /** Keyboards that were removed. */
        std::vector<Keyboard *> pulled_kbds;
        std::mutex              pulled_kbds_mtx;
        /** Controls whether or not /unseen/ keyboards may be added when they are
     * plugged in. Keyboards that were added on startup with --kbd-device
     * arguments will always be reconnected on hotplug. */
        bool                    allow_hotplug = true;

public:
        inline KBDManager() {}

        ~KBDManager();

        KBDUnlock unlockAll();

        inline void setHotplug(bool val) { allow_hotplug = val; }

        /**
     * @param path A file path or file name from /dev/input/by-id
     * @return True iff the ID represents a keyboard.
     */
        static inline bool byIDIsKeyboard(const std::string &path)
        {
                const static std::regex input_if_rx("^.*-if[0-9]+-event-kbd$");
                const static std::regex event_kbd("^.*-event-kbd$");
                // Keyboard devices in /dev/input/by-id have filenames that end in
                // "-event-kbd", but they may also have extras that are available from
                // files ending in "-ifxx-event-kbd"
                return std::regex_match(path, event_kbd) && !std::regex_match(path, input_if_rx);
        }

        /** Listen on a new device.
     *
     * @param device Full path to the device in /dev/input/
     */
        void addDevice(const std::string &device);

        /**
     * Check which keyboards have become unavailable/available again.
     */
        void updateAvailableKBDs();

        /** Set delay between outputted events in µs
     *
     * @param delay Delay in µs.
     */
        void setEventDelay(int delay);

        void startHotplugWatcher();

        void setup();

        bool getEvent(KBDAction *action);
};
