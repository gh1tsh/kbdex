/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * FSWatcher.cpp, file system monitoring.                                            *
 *                                                                                   *
 * Copyright (C) 2018 Jonas Møller (no) <jonas.moeller2@protonmail.com>              *
 * All rights reserved.                                                              *
 *                                                                                   *
 * Redistribution and use in source and binary forms, with or without                *
 * modification, are permitted provided that the following conditions are met:       *
 *                                                                                   *
 * 1. Redistributions of source code must retain the above copyright notice, this    *
 *    list of conditions and the following disclaimer.                               *
 * 2. Redistributions in binary form must reproduce the above copyright notice,      *
 *    this list of conditions and the following disclaimer in the documentation      *
 *    and/or other materials provided with the distribution.                         *
 *                                                                                   *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND   *
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED     *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE            *
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE      *
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL        *
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR        *
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER        *
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,     *
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE     *
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <sstream>
#include <functional>
#include <fstream>
#include <memory>

extern "C" {
    #include <sys/stat.h>
    #include <dirent.h>
    #include <limits.h>
    #include <string.h>
    #include <unistd.h>
    #include <poll.h>
    #include <syslog.h>
}

#include "FSWatcher.hpp"
#include "SystemError.hpp"

using namespace std;

std::atomic<int> FSWatcher::num_instances = 0;

FSWatcher::FSWatcher() {
    if ((this->fd = inotify_init()) < 0) {
        throw SystemError("Error in inotify_init()");
    }
    syslog(LOG_INFO, "Registered inotify instance %d/%d with fd: %d",
           ++num_instances, getMaxWatchers(), this->fd);
}

FSWatcher::~FSWatcher() noexcept(false) {
    stop();

    for (auto ev : events)
        delete ev;
    events.clear();

    if (close(fd) == -1)
        syslog(LOG_ERR, "Unable to close inotify file descriptor");
}

int FSWatcher::getMaxWatchers() const {
    ifstream max_inst_stream("/proc/sys/fs/inotify/max_user_instances");
    int max = 0;
    max_inst_stream >> max;
    return max;
}

void FSWatcher::stop() noexcept(false) {
    int max_wait_usec = FSW_THREAD_STOP_TIMEOUT_SEC * 1000000;
    int wait_usec = 0;
    if (running == RunState::STOPPED)
        return;
    running = RunState::STOPPING;
    while (running != RunState::STOPPED) {
        usleep(10);
        if ((wait_usec += 10) >= max_wait_usec)
            throw std::runtime_error("Unable to stop FSWatcher thread");
    }
}

void FSWatcher::add(string path) {
    // Figure out the real path to the file.
    char rpath_buf[PATH_MAX];
    const char *p = realpath(path.c_str(), rpath_buf);
    if (p == nullptr) {
        throw SystemError("Error in realpath() unable to find absolute path to: " + path);
    }
    string rpath(p);

    if (path_to_wd.find(rpath) != path_to_wd.end()) {
        // File already added, just return.
        return;
    }

    int wd = inotify_add_watch(fd, rpath.c_str(),
                               IN_MODIFY
                               | IN_DELETE
                               | IN_DELETE_SELF
                               | IN_ATTRIB
                               | IN_CREATE);
    if (wd == -1) {
        throw SystemError("Error in inotify_add_watch() for path: " + path);
    }
    path_to_wd[rpath] = wd;
    wd_to_path[wd] = rpath;
}

void FSWatcher::remove(string path) {
    char rpath_buf[PATH_MAX];
    char *p = realpath(path.c_str(), rpath_buf);
    if (p == nullptr) {
        throw SystemError("Error in realpath() unable to find absolute path to: " + path);
    }
    string rpath(rpath_buf);

    if (path_to_wd.find(rpath) == path_to_wd.end()) {
        // The file has not been added, or is being removed for a second time.
        return;
    }

    int wd = path_to_wd[rpath];
    path_to_wd.erase(rpath);
    wd_to_path.erase(wd);
    inotify_rm_watch(fd, wd);
}

vector<FSEvent> *FSWatcher::addFrom(string dir_path) {
    auto dir = shared_ptr<DIR>(opendir(dir_path.c_str()), &closedir);
    if (dir == nullptr) {
        throw SystemError("Error in opendir() for dir_path: " + dir_path);
    }
    add(dir_path);
    struct dirent *entry;
    auto added = new vector<FSEvent>();
    while ((entry = readdir(dir.get())) != nullptr) {
        stringstream ss;
        ss << dir_path << "/" << entry->d_name;
        string path = ss.str();
        struct stat stbuf;
        if (stat(path.c_str(), &stbuf) == -1) {
            throw SystemError("Unable to stat(" + path + "): ", errno);
        }
        // Only add regular files.
        if (S_ISREG(stbuf.st_mode) || S_ISLNK(stbuf.st_mode)) {
            try {
                add(path);
            } catch (SystemError &e) {
                syslog(LOG_ERR, "Unable to add path '%s': %s", path.c_str(), e.what());
                continue;
            }
            added->push_back(FSEvent(path));
        }
    }
    return added;
}

FSEvent *FSWatcher::handleEvent(struct inotify_event *ev) {
    FSEvent *fs_ev = nullptr;

    // File creation, needs to be added.
    if (ev->mask & IN_CREATE) {
        // Assemble directory and name into a full path
        string dir_path = wd_to_path[ev->wd];
        stringstream path;
        path << dir_path << "/" << ev->name;
        if (auto_add)
            try {
                add(path.str());
            } catch (SystemError &e) {
                return nullptr;
            }
        fs_ev = new FSEvent(ev, path.str());
    } else if (ev->mask & (IN_MODIFY | IN_DELETE | IN_DELETE_SELF)) {
        // File modified, save event.
        if (wd_to_path.find(ev->wd) == wd_to_path.end()) {
            throw SystemError("Received watch descriptor for file that we are not watching.");
        }
        fs_ev = new FSEvent(ev, wd_to_path[ev->wd]);
    } else if (ev->mask & (IN_ATTRIB)) {
        fs_ev = new FSEvent(ev, wd_to_path[ev->wd]);
    } else {
        return nullptr;
    }

    // Do not send events about directories.
    if (fs_ev->deleted || !S_ISDIR(fs_ev->stbuf.st_mode) || watch_dirs) {
        return fs_ev;
    }

    delete fs_ev;
    return nullptr;
}

void FSWatcher::watch(const function<bool(FSEvent &ev)> &callback) {
    // Local `running`, is set by the callback
    // Instance-wide `running` set by FSWatcher::stop()
    struct pollfd pfd;
    while (running == RunState::RUNNING) {
        pfd.fd = this->fd;
        pfd.events = POLLIN;
        poll(&pfd, 1, 128);

        try {
            // Poll with a timeout of 128 ms, this is so that we can check
            // `running` continuously.
            errno = 0;
            switch (poll(&pfd, 1, 128)) {
                case -1:
                    throw SystemError("Error in poll() on inotify fd: ", (int) errno);

                case 0:
                    // Timeout
                    break;

                default: {
                    // Acquire fs events and check for errors
                    ssize_t num_read;
                    if (this->backup_num_read == 0) {
                        num_read = read(this->fd, evbuf, sizeof(evbuf));
                        if (num_read <= 0) {
                            throw SystemError("Error in read() on inotify fd: ", (int) errno);
                        }
                    } else {
                        num_read = this->backup_num_read;
                        this->backup_num_read = 0;
                    }

                    struct inotify_event *ev;
                    // Go through received fs events.
                    for (char *p = &evbuf[0];
                         p < &evbuf[num_read];
                         p += sizeof(struct inotify_event) + ev->len)
                    {
                        ev = (struct inotify_event *) p;
                        FSEvent *fs_ev = handleEvent(ev);
                        if (fs_ev != nullptr) {
                            if (!callback(*fs_ev))
                                running = RunState::STOPPING;
                            delete fs_ev;
                        }
                        if (running != RunState::RUNNING) {
                            p += sizeof(struct inotify_event);
                            size_t len = (evbuf + sizeof(evbuf)) - p;
                            memmove(&evbuf[0], p, len);
                            this->backup_num_read = len;
                            break;
                        }
                    }
                }
            }
        } catch (const exception &e) {
            syslog(LOG_ERR, "Error: %s", e.what());
        }
    }

    this->running = STOPPED;
}

FSEvent::FSEvent(struct inotify_event *ev, string path)
    : path(path),
      mask(ev->mask)
{
    if (stat(path.c_str(), &stbuf) == -1)
        memset(&stbuf, 0, sizeof(stbuf));
    if (ev->len) {
        name = string(ev->name);
    }
}

FSEvent::FSEvent(string path) : path(path) {
    mask = 0;
    stat(path.c_str(), &stbuf);
    added = true;
}
