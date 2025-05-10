/*
 * Исходный код в этом файле частично или полностью заимствован мной из проекта
 * hawck: https://github.com/snyball/hawck/tree/af318e24e63fb1ea87cb0080fe25ee293e5f2dd0
 */

/* =====================================================================================
 * Remote user input device
 *
 * Copyright (C) 2018 Jonas Møller (no) <jonas.moeller2@protonmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * =====================================================================================
 */

#include "RemoteUDevice.hpp"

RemoteUDevice::RemoteUDevice(UNIXSocket<Packet> *conn)
{
        this->conn = conn;
}

RemoteUDevice::RemoteUDevice() {}

RemoteUDevice::~RemoteUDevice() {}

void
RemoteUDevice::emit(int type, int code, int val)
{
        Packet packet;
        memset(&packet, 0, sizeof(packet));
        packet.kbd_event.ev.type  = type;
        packet.kbd_event.ev.code  = code;
        packet.kbd_event.ev.value = val;
        evbuf.push_back(packet);
}

void
RemoteUDevice::emit(const input_event *send_event)
{
        Packet packet;
        memset(&packet, 0, sizeof(packet));
        memcpy(&packet.kbd_event.ev, send_event, sizeof(*send_event));
        packet.kbd_event.done = 0;
        evbuf.push_back(packet);
}

void
RemoteUDevice::flush()
{
        if (!conn)
                return;
        if (evbuf.size()) {
                conn->send(evbuf);
                evbuf.clear();
        }
}

void
RemoteUDevice::done()
{
        if (!conn)
                return;
        flush();
        Packet packet;
        memset(&packet, 0, sizeof(packet));
        packet.kbd_event.done = 1;
        conn->send(&packet);
}
