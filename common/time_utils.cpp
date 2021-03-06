#include "time_utils.h"
#include <iostream>
#include <poll.h>
#include <sys/timerfd.h>
#include <unistd.h>

namespace utils {
    timer_fd_t createTimer() {
        timer_fd_t timerFd;
        if ((timerFd = timerfd_create(CLOCK_MONOTONIC, 0)) == -1) {
            perror(nullptr);
            exit(EXIT_FAILURE);
        }
        return timerFd;
    }

    void setIntervalTimer(timer_fd_t timerFd, long intervalNsec) {
        timespec now{};
        if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
            perror(nullptr);
            exit(EXIT_FAILURE);
        }

        now.tv_nsec += intervalNsec;
        if (now.tv_nsec > NS_IN_SECOND) {
            now.tv_nsec -= NS_IN_SECOND;
            now.tv_sec += 1;
        }

        itimerspec timerSettings = {{0, intervalNsec}, now};
        if (timerfd_settime(timerFd, TFD_TIMER_ABSTIME, &timerSettings, nullptr) == -1) {
            perror(nullptr);
            exit(EXIT_FAILURE);
        }
    }

    void clearTimer(timer_fd_t timerFd) {
        pollfd pfds[1];
        pfds[0].fd = timerFd;
        pfds[0].events = POLLIN;
        poll(pfds, 1, 0);
        if (pfds[0].revents & POLLIN) {
            uint64_t exp;
            read(timerFd, &exp, sizeof(uint64_t));
        }
    }

    time_stamp_t getCurrentTimestamp() {
        return std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
    }

    uint64_t timestampToUll(time_stamp_t ts) {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(ts.time_since_epoch()).count());
    }
} // namespace utils
