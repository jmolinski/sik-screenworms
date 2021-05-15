#include "interval_timer.h"
#include <iostream>
#include <sys/timerfd.h>

static inline void criticalError() {
    perror(nullptr);
    exit(EXIT_FAILURE);
}

namespace utils {
    timer_fd_t createArmedTimer(long intervalNsec) {
        timespec now{};
        if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
            criticalError();
        }

        timer_fd_t timerFd;
        if ((timerFd = timerfd_create(CLOCK_MONOTONIC, 0)) == -1) {
            criticalError();
        }

        time_t intervalSec = intervalNsec / NS_IN_SECOND;
        intervalNsec = intervalNsec % NS_IN_SECOND;

        itimerspec timerSettings = {{intervalSec, intervalNsec}, now};
        if (timerfd_settime(timerFd, TFD_TIMER_ABSTIME, &timerSettings, nullptr) == -1) {
            criticalError();
        }

        return timerFd;
    }
} // namespace utils
