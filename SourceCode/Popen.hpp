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

#include <sstream>
#include <string>

extern "C" {
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
}

#include "SystemError.hpp"
#include "utils.hpp"

class Pipe
{
public:
        int io[2];

        inline Pipe()
        {
                if (pipe(io) == -1) {
                        throw SystemError("Unable to open pipe: ", errno);
                }
        }

        inline ~Pipe()
        {
                ::close(io[0]);
                ::close(io[1]);
        }

        inline int get(int idx) { return io[idx]; }

        inline void close(int idx) { ::close(io[idx]); }

        inline void dup2(int idx, int to)
        {
                if (::dup2(io[idx], to) == -1) {
                        throw SystemError("Unable to dup2(): ", errno);
                }
        }

        inline void write(int idx, const std::string &out)
        {
                if (::write(io[idx], out.c_str(), out.size()) == -1) {
                        throw SystemError("Unable to write(): ", errno);
                }
        }

        std::string read(int idx);
};

struct ArgList
{
        char **argv;

        template<class T, class... Ts>
        void collectArgvRec(char **argv, T head, Ts... tail) noexcept
        {
                std::stringstream ss;
                ss << head;
                std::string string = ss.str();
                *argv              = new char[string.size() + 1];
                void *dst          = *argv;
                ::memcpy(dst, string.c_str(), string.size() + 1);
                collectArgvRec(++argv, tail...);
        }

        inline void collectArgvRec(char **argv) noexcept { (void)argv; }

        template<class... Ts>
        char **collectArgv(Ts... args) noexcept
        {
                size_t len  = countT<Ts...>() + 1;
                char **argv = new char *[len];
                collectArgvRec(argv, args...);
                argv[len - 1] = nullptr;
                return argv;
        }

        template<class... T>
        ArgList(T... argv) noexcept
        {
                this->argv = collectArgv(argv...);
        }

        inline ~ArgList() noexcept
        {
                for (size_t i = 0; argv[i] != nullptr; i++)
                        delete[] argv[i];
                delete[] argv;
        }
};

class SubprocessError : public std::exception
{
private:
        std::string expl;

public:
        inline SubprocessError(const std::string exe, const std::string &stderr_out, int ret)
        {
                std::stringstream sstream;
                sstream << "Process " << exe << " failed with status " << ret << ": " << stderr_out;
                expl = sstream.str();
        }

        virtual const char *what() const noexcept { return expl.c_str(); }
};

/**
 * Popen with some strong assumptions about what you want, read the
 * documentation carefully.
 */
class Popen
{
private:
        Pipe        stdout;
        Pipe        stderr;
        pid_t       pid;
        std::string exe;

public:
        template<class... T>
        Popen(const std::string &exe, T... argv) : exe(exe)
        {
                ArgList args(exe, argv...);
                switch ((pid = fork())) {
                case -1:
                        throw SystemError("Unable to fork(): ", errno);

                case 0:    // In child-process
                        stdout.close(0);
                        stderr.close(0);
                        stdout.dup2(1, STDOUT_FILENO);
                        stderr.dup2(1, STDERR_FILENO);
                        if (::execvp(exe.c_str(), args.argv) == -1) {
                                std::cerr << "No such executable: " << exe;
                                abort();
                        }
                        break;

                default:
                        stdout.close(1);
                        stderr.close(1);
                }
        }

        ~Popen();

        inline void detach() noexcept { pid = -1; }

        /**
     * Read from the subprocess until it stops.
     *
     * It is assumed that if the child process exits with a status not equal to
     * 0 this indicates an error, the error is reported as a SubprocessError exception.
     * The message in the SubprocessError exception will be the stderr output of the
     * child process.
     */
        std::string readOnce();
};
