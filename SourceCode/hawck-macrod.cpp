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

#include "Daemon.hpp"
#include "MacroDaemon.hpp"
#include "XDG.hpp"
#include "utils.hpp"
#include <fstream>
#include <iostream>

#include "kbdex_config.h"

#ifndef KBDEX_VERSION
#define KBDEX_VERSION "unknown"
#endif

extern "C" {
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <syslog.h>
}

using namespace std;

static int no_fork;

int
main(int argc, char *argv[])
{
        string HELP = "Usage: kbdexCore [--no-fork]\n"
                      "\n"
                      "Options:\n"
                      "  --no-fork   Don't daemonize/fork.\n"
                      "  -h, --help  Display this help information.\n"
                      "  --version   Display version and exit.\n";
        XDG    xdg("hawck");

        //daemonize("/var/log/hawck-input/log");
        static struct option long_options[] = { /* These options set a flag. */
                                                { "no-fork", no_argument, &no_fork, 1 },
                                                { "version", no_argument, 0, 0 },
                                                /* These options don’t set a flag.
               We distinguish them by their indices. */
                                                { "help", no_argument, 0, 'h' },
                                                { 0, 0, 0, 0 }
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        unordered_map<string, function<void(const string &opt)>> long_handlers = {
                { "version",
                  [&](const string &) {
                          cout << "kbdexCore | kbdex v" KBDEX_VERSION << endl;
                          exit(0);
                  } },
        };

        do {
                int c = getopt_long(argc, argv, "h", long_options, &option_index);

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

        umask(0022);

        if (!no_fork) {
                cout << "kbdex v" KBDEX_VERSION " | kbdexCore: forking..." << endl;
                xdg.mkpath(0700, XDG_DATA_HOME, "logs");
                daemonize(xdg.path(XDG_DATA_HOME, "logs", "kbdexCore.log"));
        }

        // Kill any alread-running instance of macrod and write the new pid to the
        // pidfile.
        string pid_file = xdg.path(XDG_RUNTIME_DIR, "kbdexCore.pid");
        killPretender(pid_file);

        MacroDaemon daemon;
        try {
                daemon.run();
        } catch (exception &e) {
                cout << e.what() << endl;
                syslog(LOG_CRIT, "kbdexCore: %s", e.what());
                clearPidFile(pid_file);
                throw;
        }
}
