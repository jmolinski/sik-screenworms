#include "time_utils.h"
#include <iostream>
#include <sys/timerfd.h>

static inline void criticalError() {
    perror(nullptr);
    exit(EXIT_FAILURE);
}

namespace utils {
    timer_fd_t createArmedTimer(long intervalNsec) {
        time_t intervalSec = intervalNsec / NS_IN_SECOND;
        intervalNsec = intervalNsec % NS_IN_SECOND;

        timespec now{};
        if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
            criticalError();
        }
        now.tv_nsec += intervalNsec;
        if (now.tv_nsec > NS_IN_SECOND) {
            now.tv_nsec -= NS_IN_SECOND;
            now.tv_sec += 1;
        }

        timer_fd_t timerFd;
        if ((timerFd = timerfd_create(CLOCK_MONOTONIC, 0)) == -1) {
            criticalError();
        }

        itimerspec timerSettings = {{intervalSec, intervalNsec}, now};
        if (timerfd_settime(timerFd, TFD_TIMER_ABSTIME, &timerSettings, nullptr) == -1) {
            criticalError();
        }

        return timerFd;
    }

    time_stamp_t getCurrentTimestamp() {
        return std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
    }

    uint64_t timestampToUll(time_stamp_t ts) {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(ts.time_since_epoch()).count());
    }
} // namespace utils
