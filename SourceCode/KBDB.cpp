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

#include "KBDB.hpp"
#include "utils.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

using namespace std;
namespace fs = std::filesystem;

// As of 2020-02-22 input_id is a vector of 4 __u16 integers.
// We make assumptions based on that.
static_assert(is_same<decltype(((struct input_id *)nullptr)->bustype),
                      decltype(((struct input_id *)nullptr)->vendor)>::value);
static_assert(is_same<decltype(((struct input_id *)nullptr)->vendor),
                      decltype(((struct input_id *)nullptr)->product)>::value);
static_assert(is_same<decltype(((struct input_id *)nullptr)->product),
                      decltype(((struct input_id *)nullptr)->version)>::value);

/**
 * For unprivileged access we can't use ioctl()s, but sysfs is world-readable so
 * we can pull all the data from there.
 */
KBDInfo::KBDInfo(const struct input_id *id) : id(*id) {
    for (auto d : fs::directory_iterator("/sys/class/input")) {
        string path = d.path();
        string name = pathBasename(path);
        if (d.is_directory() && stringStartsWith(name, "event")) {
            auto id_path = pathJoin(path, "device", "id");
            struct input_id cid;
            typedef decltype(cid.vendor) id_t;
            vector<pair<string, id_t *>> parts = {{"bustype", &cid.bustype},
                                                  {"vendor", &cid.vendor},
                                                  {"product", &cid.product},
                                                  {"version", &cid.version}};

            // Read parts of the id
            for (auto [part, ptr] : parts) {
                ifstream idp(pathJoin(id_path, part));
                if (!idp.is_open())
                    throw SystemError("Unable to read: " + pathJoin(id_path, part));
                idp >> hex >> *ptr;
            }

            if (!::memcmp(&cid, id, sizeof(cid))) {
                // Found match
                initFrom(pathJoin(path, "device"));
            }
        }
    }
}

std::string KBDInfo::getID() noexcept {
    std::stringstream ss;
    ss << id.vendor << ":" << id.product << ":";
    for (char c : name) {
        if (c == ' ')
            ss << '_';
        else
            ss << c;
    }
    return ss.str();
}

void KBDInfo::initFrom(const std::string &path) noexcept(false) {
    vector<pair<string, string *>> parts = {
        {"name", &name}, {"phys", &phys}, {"uevent", &uevent}, {"device/uevent", &dev_uevent}};

    for (auto [name, ptr] : parts) {
        ifstream in(pathJoin(path, name));
        if (!in.is_open())
            throw SystemError("Unable to read: " + pathJoin(path, name));
        getline(in, *ptr);
    }

    drv = pathBasename(realpath_safe(pathJoin(path, "device", "driver")));
}

KBDB::KBDB() {}
KBDB::~KBDB() {}
