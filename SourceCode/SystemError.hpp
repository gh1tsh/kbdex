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

#include <exception>
#include <mutex>
#include <string>

extern "C" {
#undef _GNU_SOURCE
#include <string.h>
#define _GNU_SOURCE
#include <execinfo.h>
#include <stdio.h>
#include <syslog.h>
}

#include <iostream>

static std::mutex       strerror_mtx;
static constexpr size_t STACK_MAX_ELEMS = 100;

class SystemError : public std::exception
{
private:
        std::string expl;
        void       *stack[STACK_MAX_ELEMS];
        size_t      stack_sz;

        inline void _printBacktrace()
        {
                constexpr int elems = 100;
                void         *stack[elems];
                size_t        size = backtrace(stack, elems);
                backtrace_symbols_fd(stack + 1, size - 1, fileno(stderr));
        }

public:
        inline void printBacktrace() const
        {
                backtrace_symbols_fd(stack + 1, stack_sz - 1, fileno(stderr));
        }

        explicit SystemError(const std::string &expl) : expl(expl)
        {
                stack_sz = backtrace(stack, STACK_MAX_ELEMS);
        }

        inline SystemError(const std::string &expl, int errnum) : expl(expl)
        {
                this->expl += getErrorString(errnum);
                syslog(LOG_ERR, "SystemError: %s", this->expl.c_str());
        }

        static inline std::string getErrorString(int errnum)
        {
#if 0
        // FIXME: This always results in strerror_r:UNKNOWN, while
        //        the strerror function works just fine.
        char errbuf[512];
        memset(errbuf, 0, sizeof(errbuf));
        if (strerror_r(errnum, errbuf, sizeof(errbuf)) != 0) {
            switch (errno) {
                case ERANGE: strncpy(errbuf, "[strerror_r:ERANGE]", sizeof(errbuf)); break;
                case EINVAL: strncpy(errbuf, "[strerror_r:EINVAL]", sizeof(errbuf)); break;
                default:     strncpy(errbuf, "[strerror_r:UNKNOWN]", sizeof(errbuf)); break;
            }
        }
        std::string err_msg(errbuf);
#else
                // Workaround using static mutex
                std::lock_guard<std::mutex> lock(strerror_mtx);
                std::string                 err_msg(strerror(errnum));
#endif

                return err_msg;
        }

        static inline std::string getErrorString() { return getErrorString(errno); }

        virtual const char *what() const noexcept { return expl.c_str(); }
};
