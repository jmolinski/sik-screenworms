#include "utils.h"
#include <algorithm>

namespace {
    constexpr unsigned MAX_PLAYER_NAME_LENGTH = 20;
    constexpr char MIN_PLAYER_NAME_CHAR = 33;
    constexpr char MAX_PLAYER_NAME_CHAR = 126;

    bool isNonNegativeNumber(const std::string &s) {
        return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char ch) { return isdigit(ch); });
    }
} // namespace

namespace utils {
    uint64_t strToU64(const std::string &s) {
        if (!isNonNegativeNumber(s)) {
            throw std::runtime_error("invalid parameter value");
        }

        errno = 0;
        uint64_t number = std::strtoull(s.c_str(), nullptr, 10);

        if (errno == ERANGE) {
            throw std::out_of_range("invalid parameter value");
        }

        return number;
    }

    bool isValidPlayerName(const std::string &s) {
        if (s.size() > MAX_PLAYER_NAME_LENGTH) {
            return false;
        }

        for (const auto &c : s) {
            if (c < MIN_PLAYER_NAME_CHAR || c > MAX_PLAYER_NAME_CHAR) {
                return false;
            }
        }
        return true;
    }

    bool isSyntacticallyValidHostAddress(const std::string &s) {
        // TODO walidacja adresów ipv4/ipv6/aliasów?
        return !s.empty();
    }
} // namespace utils