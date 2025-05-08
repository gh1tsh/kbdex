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

extern "C" {
    #include <syslog.h>
}

#include "LuaConfig.hpp"
#include "XDG.hpp"
#include <vector>

using namespace std;
using namespace Lua;

LuaConfig::LuaConfig(const std::string& fifo_path,
                     const std::string& ofifo_path,
                     const std::string& luacfg_path)
    : FIFOWatcher(fifo_path, ofifo_path),
      luacfg_path(luacfg_path)
{
    XDG xdg("hawck");
    ChDir cd(xdg.path(XDG_DATA_HOME, "scripts", "LLib"));
    lua.from("./config.lua");
    lua.call("loadConfig", luacfg_path);
}

std::string LuaConfig::handleMessage(const char *msg, size_t) {
    try {
        auto [json] = lua.call<string>("exec", msg);
        auto [changes] = lua.call<vector<string>>("getChanged");
        lua.call("dumpConfig", luacfg_path);
        for (const auto& name : changes)
            try {
                if (option_setters.find(name) != option_setters.end())
                    option_setters[name]();
            } catch (const LuaError& e) {
                syslog(LOG_ERR, "Unable to get option %s: %s", name.c_str(), e.what());
            }
        return json;
    } catch (const LuaError& e) {
        syslog(LOG_ERR, "Lua error: %s", e.what());
    }

    return "";
}
