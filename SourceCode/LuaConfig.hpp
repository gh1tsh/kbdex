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

extern "C" {
    #include <syslog.h>
}

#include "LuaUtils.hpp"
#include "FIFOWatcher.hpp"
#include <unordered_map>

class LuaConfig : public FIFOWatcher {
private:
    Lua::Script lua;
    std::unordered_map<std::string, std::function<void()>> option_setters;
    std::string luacfg_path;

public:

    explicit LuaConfig(const std::string& fifo_path,
                       const std::string& ofifo_path,
                       const std::string& luacfg_path);

    template <class T>
    void addOption(const std::string& name, std::atomic<T> *val) {
        addOption<T>(name, [val](T got) {*val = got;});
    }

    template <class T>
    void addOption(std::string name, const std::function<void(T)> &callback) {
        option_setters[name] = [this, name, callback] {
                                   try {
                                       auto [ret] = lua.call<T>("getConfigs", name);
                                       callback(ret);
                                   } catch (const Lua::LuaError& e) {
                                       syslog(LOG_ERR,
                                              "Unable to set option %s: %s", name.c_str(), e.what());
                                   }
                               };
    }

    /** Handle the raw Lua code */
    virtual std::string handleMessage(const char *msg, size_t sz) override;
};
