#ifndef SIK_NETWORMS_TIME_UTILS_H
#define SIK_NETWORMS_TIME_UTILS_H

#include <chrono>

using timer_fd_t = int;
constexpr long NS_IN_SECOND = 1'000'000'000;
constexpr long NS_IN_MS = 1'000'000;
constexpr long US_IN_SECOND = 1'000'000;

namespace utils {
    using time_stamp_t = std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>;

    timer_fd_t createArmedTimer(long intervalMs);
    time_stamp_t getCurrentTimestamp();
    uint64_t timestampToUll(time_stamp_t ts);
} // namespace utils

#endif // SIK_NETWORMS_TIME_UTILS_H
