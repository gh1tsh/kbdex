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

#include <filesystem>
#include <fstream>
#include <string>

#include "Daemon.hpp"
#include "KBDDaemon.hpp"
#include "utils.hpp"

#include "kbdex_config.h"

#ifndef KBDEX_VERSION
#define KBDEX_VERSION "unknown"
#endif

extern "C" {
#include <getopt.h>
#include <signal.h>
#include <syslog.h>
}

using namespace std;
namespace fs = std::filesystem;

static void
handleSigPipe(int)
{
        // cout << "KBDDaemon aborting due to SIGPIPE" << endl;
        // abort();
}

static int no_fork;

auto
varToOption(string opt)
{
        replace(opt.begin(), opt.end(), '_', '-');
        return opt;
}

#define VAR_TO_OPTION(_var) varToOption(#_var)

auto
numOption(const string &name, int *num)
{
        return [num, name](const string &opt) {
                try {
                        *num = stoi(opt);
                } catch (const exception &e) {
                        cout << "--" << name << ": Require an integer" << endl;
                        exit(0);
                }
        };
}

auto
strOption(const string &, string *str)
{
        return [str](const string &opt) {
                *str = opt;
        };
}

#define NUM_OPTION(_name) { VAR_TO_OPTION(_name), numOption(VAR_TO_OPTION(_name), &(_name)) },
#define STR_OPTION(_name) { VAR_TO_OPTION(_name), strOption(VAR_TO_OPTION(_name), &(_name)) }

int
main(int argc, char *argv[])
{
        signal(SIGPIPE, handleSigPipe);

        string HELP =
            "Usage: kbdexKeyboardAgent [--udev-event-delay <us>] [--no-fork] [--socket-timeout]\n"
            "                    [--kbd-device <device>] [--no-hotplug]\n"
            "\n"
            "Examples:\n"
            "  Listen on a single device:\n"
            "    kbdexKeyboardAgent --kbd-device /dev/input/event13 --no-hotplug\n\n"
            "  Listen on multiple devices:\n"
            "    kbdexKeyboardAgent -k{/dev/input/event13,/dev/input/event15}\n\n"
            "  Listen on all keyboard devices automatically:\n"
            "    kbdexKeyboardAgent\n\n"
            "Options:\n"
            "  --no-fork           Don't daemonize/fork.\n"
            "  -h, --help          Display this help information.\n"
            "  --version           Display version and exit.\n"
            "  -k, --kbd-device    Add a keyboard to listen to.\n"
            "  --udev-event-delay  Delay between events sent on the udevice in µs.\n"
            "  --socket-timeout    Time in milliseconds until timeout on sockets.\n"
            "  --no-hotplug        Only listen to devices that were explicitly added with "
            "--kbd-device\n";

        int                  no_hotplug     = false;
        static struct option long_options[] = {
                /* These options set a flag. */
                { "no-fork", no_argument, &no_fork, 1 },
                { "no-hotplug", no_argument, &no_hotplug, 1 },
                { "udev-event-delay", required_argument, 0, 0 },
                { "socket-timeout", required_argument, 0, 0 },
                { "version", no_argument, 0, 0 },
                /* These options don’t set a flag.
                   We distinguish them by their indices. */
                { "help", no_argument, 0, 'h' },
                { "kbd-device", required_argument, 0, 'k' },
                { "kbd-name", required_argument, 0, 'n' },
                { 0, 0, 0, 0 }
        };

        int option_index     = 0; /* getopt_long stores the option index here. */
        int udev_event_delay = 3800;
        int socket_timeout   = 1024;
        vector<string> kbd_names;
        vector<string> kbd_devices;

        unordered_map<string, function<void(const string &opt)>> long_handlers = {
                { "version",
                  [&](const string &) {
                          cout << "kbdex v" KBDEX_VERSION << endl;
                          exit(0);
                  } },
                NUM_OPTION(udev_event_delay) NUM_OPTION(socket_timeout)
        };

        do {
                int c = getopt_long(argc, argv, "hk:n:", long_options, &option_index);

                /* Detect the end of the options. */
                if (c == -1)
                        break;

                switch (c) {
                case 0: {
                        /* If this option set a flag, do nothing else now. */
                        if (long_options[option_index].flag != 0)
                                break;
                        string name(long_options[option_index].name);
                        string arg(optarg ? optarg : "");
                        if (long_handlers.find(name) != long_handlers.end())
                                long_handlers[name](arg);
                        break;
                }

                case 'k':
                        kbd_devices.push_back(string(optarg));
                        break;

                // TODO: Implement lookup of keyboard names
                case 'n':
                        cout << "Option -n/--kbd-name: Not implemented." << endl;
                        break;

                case 'h':
                        cout << HELP;
                        exit(0);

                case '?':
                        /* getopt_long already printed an error message. */
                        break;

                default:
                        cout << "DEFAULT" << endl;
                        abort();
                }
        } while (true);

        // If no devices were specified, we listen to all of them
        if (kbd_devices.size() == 0) {
                // FIXME: All the "is this a keyboard" detection done in Hawck is very
                //        brittle, and I should probably do a deep-dive into whatever
                //        documentation I can find on this to develop a better method.

                try {
                        for (auto d : fs::directory_iterator("/dev/input/by-id"))
                                if (KBDManager::byIDIsKeyboard(d.path()))
                                        kbd_devices.push_back(realpath_safe(d.path()));
                } catch (const system_error &err) {
                        syslog(LOG_ERR, "Unable to query by-id: %s", err.what());
                }

                try {
                        for (auto d : fs::directory_iterator("/dev/input/by-path")) {
                                string rpath = realpath_safe(d.path());
                                bool   already_added =
                                    find(kbd_devices.begin(), kbd_devices.end(), rpath) !=
                                    kbd_devices.end();
                                if (!already_added &&
                                    stringStartsWith(pathBasename(d.path()), "platform-i8042") &&
                                    stringEndsWith(d.path(), "-event-kbd"))
                                        kbd_devices.push_back(rpath);
                        }
                } catch (const system_error &err) {
                        syslog(LOG_ERR, "Unable to query by-path: %s", err.what());
                }
        }

        cout << "Starting kbdexKeyboardAgent on:" << endl;
        for (const auto &dev : kbd_devices)
                cout << "  - <" << dev << ">" << endl;

        if (!no_fork) {
                cout << "forking ..." << endl;
                daemonize("/tmp/kbdexKeyboardAgent.log");
        }

        const string pid_file = "/var/lib/kbdexKeyboardAgent/pid";
        killPretender(pid_file);

        try {
                KBDDaemon daemon;
                daemon.kbman.setHotplug(!no_hotplug);
                for (const auto &dev : kbd_devices)
                        daemon.kbman.addDevice(dev);
                daemon.setEventDelay(udev_event_delay);
                daemon.setSocketTimeout(socket_timeout);
                syslog(LOG_INFO, "Running kbdexKeyboardAgent ...");
                daemon.run();
        } catch (const SystemError &e) {
                syslog(LOG_CRIT, "kbdexKeyboardAgent aborted due to exception: %s", e.what());
                cout << "Error: " << e.what() << endl;
                e.printBacktrace();
                clearPidFile(pid_file);
                throw;
        }
}
