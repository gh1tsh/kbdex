extern "C" {
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
}

#include <chrono>
#include <thread>

#include "FIFOWatcher.hpp"
#include "UNIXSocket.hpp"
#include "utils.hpp"

using namespace std;

FIFOWatcher::FIFOWatcher(const std::string& ififo_path, const std::string& ofifo_path)
    : path(ififo_path),
      opath(ofifo_path)
{}

FIFOWatcher::~FIFOWatcher() {
    if (fd != -1)
        ::close(fd);
    if (ofd != -1)
        ::close(ofd);
}

void FIFOWatcher::reply(std::string buf, Milliseconds timeout) {
    // Time increment in milliseconds
    static constexpr int time_inc = 10;

    // Attempt to open the file
    for (int time = 0, ofd = -1; time < timeout.count(); time++) {
        if ((ofd = ::open(opath.c_str(), O_WRONLY | O_NONBLOCK)) != -1) {
            uint32_t bufsz = buf.size();
            if (::write(ofd, &bufsz, sizeof(bufsz)) == -1 ||
                ::write(ofd, buf.c_str(), bufsz) == -1)
            {
                throw SystemError("Unable to write to out-fifo \"" + opath + "\": ", errno);
            }
            return;
        }

        // Sleep time_inc milliseconds
        usleep(1000 * time_inc);
    }

    throw SystemError("Unable to open out-fifo \"" + opath + "\": ", errno);
}

void FIFOWatcher::reset() {
    syslog(LOG_DEBUG, "Resetting fifo");

    if (fd != -1)
        close(fd);

    if ((fd = ::open(path.c_str(), O_RDONLY)) == -1) {
        throw SystemError("Unable to open fifo: ", errno);
    }
}

void FIFOWatcher::watch() {
    uint32_t sz;

    reset();

    for (;;) {
        try {
            recvAll(fd, &sz);
            if (sz >= max_recv || sz == 0) {
                reset();
                continue;
            }
            auto buf = unique_ptr<char[]>(new char[sz + 1]);
            buf.get()[sz] = 0;
            recvAll(fd, buf.get(), sz, std::chrono::milliseconds(256));
            auto ret = handleMessage(buf.get(), sz);
            reply(ret, Milliseconds(256));
        } catch (const SocketError& e) {
            reset();
        } catch (const SystemError& e) {
            reset();
        }
    }
}

// Example implementation
std::string FIFOWatcher::handleMessage(const char *buf, size_t) {
    syslog(LOG_DEBUG, "Got message: %s", buf);
    return "";
}

void FIFOWatcher::start() {
    thread t0([this]() {
                  watch();
              });
    t0.detach();
}
