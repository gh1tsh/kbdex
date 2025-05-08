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

#include "Popen.hpp"

using namespace std;

Popen::~Popen() {
    int status;
    if (pid != -1)
        waitpid(pid, &status, 0);
}

string Popen::readOnce() {
    stringstream sstream;
    char buf[8192];
    int nb, status;

    do {
        if ((nb = ::read(stdout.io[0], buf, sizeof(buf))) == -1)
            throw SystemError("Unable to read: ", errno);
        sstream.write(buf, nb);
    } while (waitpid(pid, &status, WNOHANG) != -1);

    pid = -1;
    if (status != 0)
        throw SubprocessError(exe, stderr.read(0), status);
    return sstream.str();
}

string Pipe::read(int idx) {
    stringstream sstream;
    char buf[8192];
    int nb;

    do {
        if ((nb = ::read(io[idx], buf, sizeof(buf))) == -1)
            throw SystemError("Unable to read(): ", errno);
        sstream.write(buf, nb);
    } while (nb > 0);

    return sstream.str();
}
