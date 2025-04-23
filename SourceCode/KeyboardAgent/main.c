#include "kbdex_generic_c_header.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <libevdev/libevdev.h>
#include <libinput.h>
#include <libudev.h>
#include <signal.h>
#include <sys/stat.h>
#include <syslog.h>

#define BD_MAX_CLOSE 8192

volatile Boolean running = FALSE;

/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2024.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU Lesser General Public License as published   *
* by the Free Software Foundation, either version 3 or (at your option)   *
* any later version. This program is distributed without any warranty.    *
* See the files COPYING.lgpl-v3 and COPYING.gpl-v3 for details.           *
\*************************************************************************/

/*
    Исходный код функции BecomeDaemon() основан на листингах 37-1, 37-2 из книги
    "The Linux Programming Interface" (Michael Kerrisk).
*/

#include "kbdex_generic_c_header.h"

/**
 * @return 0 в случае успешного выполнения, -1 в случае ошибки.
 */
int
become_daemon()
{
        long maxfd;
        int  fd;

        switch (fork()) { /* Become background process */
        case -1:
                return -1;
        case 0:
                break; /* Child falls through... */
        default:
                _exit(EXIT_SUCCESS); /* while parent terminates */
        }

        if (setsid() == -1) /* Become leader of new session */
                return -1;

        switch (fork()) { /* Ensure we are not session leader */
        case -1:
                return -1;
        case 0:
                break;
        default:
                _exit(EXIT_SUCCESS);
        }

        umask(0); /* Clear file mode creation mask */

        /* Change to root directory */
        if (chdir("/") == -1) {
                // Не удалось сменить рабочий каталог на корневой каталог. Аварийно
                // завершаемся.
                return -1;
        }

        /* Close all open files */
        maxfd = sysconf(_SC_OPEN_MAX);
        if (maxfd == -1)              /* Limit is indeterminate... */
                maxfd = BD_MAX_CLOSE; /* so take a guess */

        for (fd = 0; fd < maxfd; fd++)
                close(fd);

        close(STDIN_FILENO); /* Reopen standard fd's to /dev/null */

        fd = open("/dev/null", O_RDWR);

        if (fd != STDIN_FILENO) /* 'fd' should be 0 */
                return -1;
        if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
                return -1;
        if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
                return -1;

        return 0;
}

void
signal_handler(int signum)
{
        running = FALSE;
}

static int
open_restricted(const char *path, int flags, void *user_data)
{
        int fd = open(path, flags);
        if (fd < 0)
                fprintf(stderr, "Failed to open %s because of %s.\n", path, strerror(errno));
        return fd < 0 ? -errno : fd;
}

static void
close_restricted(int fd, void *user_data)
{
        close(fd);
}

static const struct libinput_interface libinputInterface = {
        .open_restricted  = open_restricted,
        .close_restricted = close_restricted,
};

int
main(int argc, char **argv)
{
        signal(SIGTERM, signal_handler);
        signal(SIGINT, signal_handler);
        signal(SIGHUP, signal_handler);
        signal(SIGQUIT, signal_handler);
        signal(SIGKILL, signal_handler);

        // rc от "return code" т.е. "код возврата".
        int rc;

        // NOTE: возможно в параметре __option стоит указать LOG_NDELAY.
        openlog(argv[0], 0, LOG_USER);

        rc = become_daemon();

        if (rc == 0) {
                syslog(LOG_INFO, "kbdex | KeyboardAgent инициализирован успешно");

                running = TRUE;

                struct libinput *libinput_context;
                struct udev     *udev_context;

                syslog(LOG_INFO, "kbdex | Инициализация udev...");

                udev_context = udev_new();
                if (udev_context == NULL) {
                        syslog(LOG_ERR, "kbdex | Не удалось выполнить инициализацию udev");
                        syslog(LOG_INFO, "kbdex | Завершение работы демона susuwatari");

                        running = FALSE;

                        closelog();
                        // exit(3)
                        exit(EXIT_FAILURE);
                }

                syslog(LOG_INFO, "kbdex | Инициализация udev выполнена успешно");

                syslog(LOG_INFO, "kbdex | Инициализация libinput...");

                libinput_context =
                    libinput_udev_create_context(&libinputInterface, NULL, udev_context);
                if (libinput_context == NULL) {
                        syslog(LOG_ERR, "kbdex | Не удалось выполнить инициализацию libinput");
                        syslog(LOG_INFO, "kbdex | Завершение работы демона KeyboardAgent");

                        running = FALSE;
                        udev_unref(udev_context);
                        closelog();
                        // exit(3)
                        exit(EXIT_FAILURE);
                }

                if (libinput_udev_assign_seat(libinput_context, "seat0") != 0) {
                        syslog(
                            LOG_ERR,
                            "kbdex | Не удалось выполнить присвоение сессии (seat0) для libinput");
                        syslog(LOG_INFO, "kbdex | Завершение работы демона KeyboardAgent");

                        running = FALSE;
                        libinput_unref(libinput_context);
                        udev_unref(udev_context);
                        closelog();
                        // exit(3)
                        exit(EXIT_FAILURE);
                }

                syslog(LOG_INFO, "kbdex | Инициализация libinput выполнена успешно");

                syslog(LOG_DEBUG, "kbdex | Запуск главного цикла");

                struct libinput_event *liEvent;

                while (running) {
                        if (libinput_dispatch(libinput_context) == 0) {
                                while ((liEvent = libinput_get_event(libinput_context)) != NULL) {
                                        if (libinput_event_get_type(liEvent) ==
                                            LIBINPUT_EVENT_KEYBOARD_KEY) {
                                                struct libinput_event_keyboard *kbd_event =
                                                    libinput_event_get_keyboard_event(liEvent);

                                                enum libinput_key_state key_state =
                                                    libinput_event_keyboard_get_key_state(
                                                        kbd_event);

                                                uint32_t key_code =
                                                    libinput_event_keyboard_get_key(kbd_event);

                                                const char *key_name =
                                                    libevdev_event_code_get_name(EV_KEY, key_code);

                                                syslog(LOG_DEBUG,
                                                       "kbdex | Клавиша %s %s",
                                                       key_name,
                                                       key_state == LIBINPUT_KEY_STATE_PRESSED
                                                           ? "нажата"
                                                           : "отпущена");
                                        }

                                        libinput_event_destroy(liEvent);
                                }
                        } else {
                                syslog(LOG_ERR, "kbdex | Ошибка при выполнении libinput_dispatch");
                        }
                }

                syslog(LOG_DEBUG, "kbdex | Завершение работы главного цикла");
                syslog(LOG_INFO, "kbdex | Завершение работы демона KeyboardAgent");

                libinput_unref(libinput_context);
                udev_unref(udev_context);
                closelog();
                // exit(3)
                exit(EXIT_SUCCESS);
        } else {
                // Произошла ошибка при попытке превращения процесса в процесс-демон.
                // Аварийно завершаем работу программы.
                syslog(LOG_ERR,
                       "Не удалось выполнить инициализацию демона KeyboardAgent. Ошибка: %s",
                       strerror(errno));
                syslog(LOG_INFO, "Завершение работы демона KeyboardAgent");

                closelog();
                // exit(3)
                exit(EXIT_FAILURE);
        }
}