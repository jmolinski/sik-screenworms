#ifndef SIK_NETWORMS_MISC_H
#define SIK_NETWORMS_MISC_H

#include <cstdint>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace config {
    constexpr unsigned MAX_PLAYER_NAME_LENGTH = 20;
}

namespace utils {
    uint64_t strToU64(const std::string &s);

    template <typename T, T minValue, T maxValue>
    T optionalParamValueToUint(const std::unordered_map<char, std::string>::iterator &paramIt) {
        try {
            uint64_t num = strToU64(paramIt->second);
            T castNum = static_cast<T>(num);
            if (num > std::numeric_limits<T>::max() || castNum > maxValue || castNum < minValue) {
                std::ostringstream os;
                os << '[' << minValue << "; " << maxValue << ']';
                throw std::out_of_range("value out of allowed range " + os.str());
            }

            return castNum;
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string(1, paramIt->first) + ": " + std::string(e.what()));
        }
    }

    bool isValidPlayerName(const std::string &s);
} // namespace utils

#endif // SIK_NETWORMS_MISC_H
