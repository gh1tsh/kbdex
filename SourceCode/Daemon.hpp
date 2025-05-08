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

/** @file Daemon.hpp
 *
 * @brief Daemonization utilities.
 */

#include <string>

/**
 * Change stdout and stderr to point to different files, specified
 * by the given paths.
 *
 * Stdin will always be redirected from /dev/null, if /dev/null cannot
 * be open this will result in abort(). Note that after this call any
 * blocking reads on stdin will halt the program.
 *
 * @param stdout_path File to redirect stdout into.
 * @param stderr_path File to redirect stderr into.
 */
void dup_streams(const std::string &stdout_path,
                 const std::string &stderr_path);

/** Turn the calling process into a daemon.
 */
void daemonize();

/** Turn the calling process into a daemon.
 *
 * @param logfile_path The logfile to redirect stderr and stdout
 *                     into.
 */
void daemonize(const std::string &logfile_path);

/**
 * Kill process according to pid_file.
 *
 * @param pid_file Path to a file where the pid of each new daemon is stored.
 *
 * When `killPretender` is finished it will write `getpid()` to the pid file.
 *
 * The process corresponding to the pid in the pid file *must* have the same
 * basename exe as the process calling `killPretender`.
 */
void killPretender(std::string pid_file);

/**
 * Clear pid file, but only if it contains the pid for the process calling
 * `cleanPidFile`.
 *
 * @param pid_file Path to a file where the pid of each new daemon is stored.
 */
void clearPidFile(std::string pid_file);
