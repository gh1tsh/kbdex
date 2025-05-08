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
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <stdlib.h>
    #include <stdio.h>
    #include <sys/stat.h>
    #include <signal.h>
    #include <sys/types.h>
    #include <syslog.h>
    #include <limits.h>
}

#include <string>
#include <fstream>
#include <memory>

#include "SystemError.hpp"
#include "Daemon.hpp"
#include "utils.hpp"

#if MESON_COMPILE
#include <hawck_config.h>
#endif

using namespace std;

static constexpr size_t BD_MAX_CLOSE = 8192;

void dup_streams(const string &stdout_path, const string &stderr_path) {
    using CFile = unique_ptr<FILE, decltype(&fclose)>;
    auto fopen = [](string path, string mode) -> CFile {
                     return CFile(::fopen(path.c_str(), mode.c_str()), fclose);
                 };

    // Attempt to open /dev/null
    auto dev_null = fopen("/dev/null", "r");
    if (dev_null == nullptr)
        abort();

    // Open new output files
    auto stdout_new = fopen(stdout_path, "wa");
    auto stderr_new = fopen(stderr_path, "wa");
    if (stdout_new == nullptr || stderr_new == nullptr) {
        throw invalid_argument("Unable to open new stdout and stderr");
    }

    // Dup files
    ::dup2(STDIN_FILENO, dev_null->_fileno);
    ::dup2(STDOUT_FILENO, stdout_new->_fileno);
    ::dup2(STDERR_FILENO, stderr_new->_fileno);
}

/**
 * Fork through, i.e fork and become the subprocess while the original process
 * stops.
 *
 * @throws SystemError If the fork() call fails.
 */
inline void forkThrough() {
    switch (fork()) {
        case -1: throw SystemError("Unable to fork(): ", errno);
        case 0: break;
        default: exit(0);
    }
}

/**
 * Adapted to C++ from Michael Kerrisks TLPI book.
 */
void daemonize(const string &logfile_path) {
    forkThrough();

    if (setsid() == -1)
        throw SystemError("Unable to setsid(): ", errno);

    forkThrough();

    umask(0);

    if (chdir("/") == -1)
        throw SystemError("Unable to chdir(\"/\"): ", errno);

    // Close all files
    int maxfd = sysconf(_SC_OPEN_MAX);
    maxfd = (maxfd == -1) ? BD_MAX_CLOSE : maxfd;
    for (int fd = 0; fd < maxfd; fd++)
        close(fd);

    dup_streams(logfile_path, logfile_path);
}

/**
 * Get path to the executable
 */
static std::string pidexe(pid_t pid) {
    char buf[PATH_MAX];
    // The documentation for readlink wasn't clear on whether it would
    // write an empty string on error.
    memset(buf, '\0', sizeof(buf));
    if (readlink(pathJoin("/proc", pid, "exe").c_str(), buf, sizeof(buf)) == -1)
        (void) 0; // Ignore
    return std::string(buf);
}

void killPretender(std::string pid_file) {
    Flocka flock(pid_file);
    int old_pid = 0; {
        std::ifstream pidfile(pid_file);
        pidfile >> old_pid;
    }
    std::string old_exe = pidexe(old_pid);
    if (pathBasename(old_exe) == pathBasename(pidexe(getpid()))) {
        syslog(LOG_WARNING, "Killing previous %s instance ...",
               old_exe.c_str());
        kill(old_pid, SIGTERM);
    } else {
        syslog(LOG_INFO, "Outdated pid file with exe '%s', continuing ...",
               old_exe.c_str());
    }
    std::ofstream pidfile_out(pid_file);
    pidfile_out << getpid() << std::endl;
}

void clearPidFile(std::string pid_file) {
    Flocka flock(pid_file);
    int pid; {
        std::ifstream pidfile(pid_file);
        pidfile >> pid;
    }
    if (pid == getpid()) {
        std::ofstream pidfile(pid_file);
        pidfile << std::endl;
    }
}

void daemonize() {
    daemonize("/dev/null");
}
