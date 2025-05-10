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

#include <string>
#include <unordered_map>

extern "C" {
#include <linux/input.h>
#include <linux/uinput.h>
}

struct InputIDHash
{
        std::size_t operator()(const struct input_id &k) const
        {
                using std::hash;
                return ((hash<int>()(k.bustype) ^ (hash<int>()(k.vendor) << 1)) >> 1) ^
                       (hash<int>()(k.product) << 1 ^ (hash<int>()(k.version) << 3));
        }
};

inline bool
operator==(const struct input_id &a, const struct input_id &b)
{
        return (a.bustype == b.bustype) && (a.product == b.product) && (a.vendor == b.vendor) &&
               (a.version == b.version);
}

/**
 * TODO: Refactor the Keyboard class to use KBDInfo
 */
class KBDInfo
{
private:
        std::string     name;
        std::string     drv;
        std::string     phys;
        std::string     uevent;
        std::string     dev_uevent;
        struct input_id id;

        void initFrom(const std::string &path) noexcept(false);

public:
        KBDInfo(const struct input_id *id) noexcept(false);

        inline KBDInfo() noexcept {}

        std::string getID() noexcept;
};

/**
 * Keyboard database, accepts `struct input_id` and retrieves the rest of the
 * information from scanning sysfs.
 *
 * Currently its only used to manage human-readable keyboard IDs.
 */
class KBDB
{
private:
        std::unordered_map<struct input_id, KBDInfo, InputIDHash> info;

public:
        KBDB();
        ~KBDB();

        std::string getID(const struct input_id *id) noexcept(false)
        {
                if (info.count(*id) == 0) {
                        KBDInfo kinfo(id);
                        info[*id] = kinfo;
                        return kinfo.getID();
                }
                return info[*id].getID();
        }
};
