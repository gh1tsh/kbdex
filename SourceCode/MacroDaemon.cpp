/*
 * Исходный код в этом файле частично или полностью заимствован мной из проекта
 * hawck: https://github.com/snyball/hawck/tree/af318e24e63fb1ea87cb0080fe25ee293e5f2dd0
 */

/* =====================================================================================
 * Macro daemon.
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
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS     OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * =====================================================================================
 */

#include <filesystem>
#include <iostream>
#include <thread>

extern "C" {
#include <libnotify/notify.h>
#include <syslog.h>
}

#include "Daemon.hpp"
#include "Decoder.hpp"
#include "KBDB.hpp"
#include "Language.hpp"
#include "MacroDaemon.hpp"
#include "Permissions.hpp"
#include "Popen.hpp"
#include "XDG.hpp"
#include "utils.hpp"

using namespace Permissions;
using namespace std;
namespace fs = std::filesystem;

static bool kbdexCore_main_loop_running = true;

MacroDaemon::MacroDaemon() : kbd_srv("/var/lib/kbdex/kbd.sock"), xdg("kbdex")
{
        notify_on_err = true;
        stop_on_err   = false;
        eval_keydown  = true;
        eval_keyup    = true;
        eval_repeat   = true;
        disabled      = false;

        auto [grp, grpbuf] = getgroup("kbdex-input-share");
        (void)grpbuf;
        if (chown("/var/lib/kbdex/kbd.sock", getuid(), grp->gr_gid) == -1)
                throw SystemError("Unable to chown kbd.sock: ", errno);
        if (chmod("/var/lib/kbdex/kbd.sock", 0660) == -1)
                throw SystemError("Unable to chmod kbd.sock: ", errno);
        notify_init("kbdex");
}

void
MacroDaemon::getConnection()
{
        if (kbd_com)
                delete kbd_com;
        syslog(LOG_INFO, "Listening for a connection ...");

        // Keep looping around until we get a connection.
        for (;;) {
                try {
                        int fd  = kbd_srv.accept();
                        kbd_com = new UNIXSocket<Packet>(fd);
                        syslog(LOG_INFO, "Got a connection");
                        break;
                } catch (SocketError &e) {
                        syslog(LOG_ERR, "Error in accept(): %s", e.what());
                }
                // Wait for 0.1 seconds
                usleep(100000);
        }

        remote_udev.setConnection(kbd_com);
}

MacroDaemon::~MacroDaemon()
{}

void
MacroDaemon::notify(string title, string msg)
{
        notify(title, msg, "kbdex", NOTIFY_URGENCY_NORMAL);
}

void
MacroDaemon::notify(string title, string msg, string icon, NotifyUrgency urgency)
{
        // AFAIK you don't have to free the memory manually, but I could be wrong.
        lock_guard<mutex>     lock(last_notification_mtx);
        tuple<string, string> notif(title, msg);
        if (notif == last_notification)
                return;
        last_notification = notif;

        NotifyNotification *n = notify_notification_new(title.c_str(), msg.c_str(), icon.c_str());
        notify_notification_set_timeout(n, 10000);
        notify_notification_set_urgency(n, urgency);
        notify_notification_set_app_name(n, "kbdex");

        if (!notify_notification_show(n, nullptr)) {
                syslog(LOG_INFO, "Notifications cannot be shown.");
        }
}

static void
handleSigPipe(int)
{}

#if 0
static void handleSigTerm(int) {
    kbdexCore_main_loop_running = false;
}
#endif

void
MacroDaemon::reloadAll()
{
// Disabled due to restructuring of the script load process
#if 0
    lock_guard<mutex> lock(scripts_mtx);
    auto chdir = xdg.cd(XDG_DATA_HOME, "scripts");
    for (auto &[_, sc] : scripts) {
        (void) _;
        try {
            sc->setEnabled(true);
            sc->reset();
            sc->call("require", "init");
            sc->open(&remote_udev, "udev");
            sc->reload();
        } catch (const LuaError& e) {
            syslog(LOG_ERR, "Error when reloading script: %s", e.what());
            sc->setEnabled(false);
        }
    }
#endif
}

void
MacroDaemon::processKbdEvent(const Packet& packet)
{
        this->kbd_events_buffer.push_back(packet);

        int keycode = packet.kbd_event.ev.value;

        char repr = Decoder::getKeycodeRepr(keycode);

        if (Decoder::isWhitespace(keycode)) {
                this->processBuffers();
                return;
        }

        if (Decoder::isLetter(keycode) || Decoder::isDigit(keycode)) {
                this->char_buffer.push_back(repr);
        }
}

void
MacroDaemon::processCmd(const Packet& packet)
{
        return;
}

void
MacroDaemon::processBuffers()
{
        this->kbd_events_buffer.clear();

        std::string content;

        for (char c : this->char_buffer) {
                content += c;
        }

        // На данном этапе просто посмотрим, как работает обработка вводимых символов.
        std::cout << content << std::endl;

        this->char_buffer.clear();
}

void
MacroDaemon::run()
{
        syslog(LOG_INFO, "kbdex v%s | kbdexCore: Setting up kbdexCore ...", KBDEX_VERSION);
#ifdef MODE_COMMUNICATION_CHECK
        syslog(LOG_INFO, "kbdex v%s | kbdexCore: Функционирование в режиме проверки связи", KBDEX_VERSION);
#endif
        // FIXME: Need to handle socket timeouts before I can use this SIGTERM handler.
        //signal(SIGTERM, handleSigTerm);

        signal(SIGPIPE, handleSigPipe);


        Packet              packet;
        struct input_event &ev = packet.kbd_event.ev;
        KBDB                kbdb;

#ifdef MODE_COMMUNICATION_CHECK
        bool pingSentFlag = false;
#endif

        getConnection();

        syslog(LOG_INFO, "kbdex v%s | kbdexCore: Starting main loop", KBDEX_VERSION);

#ifdef MODE_COMMUNICATION_CHECK
        syslog(LOG_INFO, "kbdex v%s | kbdexCore: Подготовка к отправке команды PING", KBDEX_VERSION);
#endif

        while (kbdexCore_main_loop_running) {
                try {
#ifdef MODE_COMMUNICATION_CHECK
                        // kbdexCore запускается первым, поэтому он будет отправлять команду
                        // PING и получать команду PONG.
                        memset(&packet, '\0', sizeof(packet));

                        if (!pingSentFlag) {
                                packet.type = PacketType::Command;
                                packet.cmd.command = Command::PING;
                                strncpy(packet.cmd.payload, "TEST", 512);

                                kbd_com->send(&packet);

                                pingSentFlag = true;

                                syslog(LOG_INFO, "kbdex v%s | kbdexCore: Команда PING отправлена. Ожидание команды PONG от kbdexKeyboardAgent...", KBDEX_VERSION);
                        }

                        if (pingSentFlag) {
                                memset(&packet, '\0', sizeof(packet));
                                // После отправки PING ожидаем команду PONG в ответ.
                                kbd_com->recv(&packet);

                                pingSentFlag = false;
                        }

                        if (packet.type == PacketType::Command) {
                                syslog(LOG_INFO, "kbdex v%s | kbdexCore: Получена команда PONG c данными '%s'", KBDEX_VERSION, packet.cmd.payload);
                        }

                        // Отправляем и получаем команды раз в 5 секунд
                        sleep(5);
#else
                        bool repeat = true;

                        kbd_com->recv(&packet);

                        if (packet.type == PacketType::Command) {
                                // TODO: реализовать обработку команд
                                continue;
                        } else if (packet.type == PacketType::KeyboardEvent) {
                                processKbdEvent(packet);
                                /*
                                string kbd_hid = kbdb.getID(&packet.kbd_event.dev_id);

                                if (!((!eval_keydown && ev.value == 1) ||
                                    (!eval_keyup && ev.value == 0)) && !disabled) {
                                        // Выполнение операций над событиями клавиатуры с мьютексом.
                                        std::cout << "\nПолучено событие клавиатуры от kbdexKeyboardAgent:\n"
                                                  << "[ done: " << packet.kbd_event.done << "\t"
                                                  << "event: { " << ev.value << " " << ev.code
                                                  << " " << ev.type << " } ]" << std::endl;
                                }

                                if (repeat)
                                        remote_udev.emit(&ev);

                                remote_udev.done();
                                */
                        }
#endif
                } catch (const SocketError &e) {
                        // Reset connection
                        syslog(LOG_ERR, "Socket error: %s", e.what());
                        notify("Socket error",
                               "Connection to kbdexKeyboardAgent timed out, reconnecting ...",
                               "kbdex",
                               NOTIFY_URGENCY_NORMAL);
                        getConnection();
                }
        }

        syslog(LOG_INFO, "kbdex v%s | kbdexCore exiting ...", KBDEX_VERSION);
}
