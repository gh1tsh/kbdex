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
