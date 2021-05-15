#ifndef SIK_NETWORMS_INTERVAL_TIMER_H
#define SIK_NETWORMS_INTERVAL_TIMER_H

using timer_fd_t = int;
constexpr long NS_IN_SECOND = 1'000'000'000;

namespace utils {
    timer_fd_t createArmedTimer(long intervalMs);
}

#endif // SIK_NETWORMS_INTERVAL_TIMER_H
