#ifndef SIK_NETWORMS_MESSAGES_H
#define SIK_NETWORMS_MESSAGES_H

#include "misc.h"
#include <exception>

class EncoderDecoderError : public std::exception {};

enum struct TurnDirection : uint8_t { straight = 0, right = 1, left = 2 };

struct ClientToServerMessage {
    static constexpr size_t min_struct_size = 13, max_struct_size = 33;

    uint64_t sessionId;
    TurnDirection turnDirection;
    uint32_t nextExpectedEventNo;
    char playerName[config::MAX_PLAYER_NAME_LENGTH + 1];
    size_t playerNameSize;

    ClientToServerMessage(const unsigned char *buffer, size_t size);
    size_t encode(unsigned char *buffer);
};

#endif // SIK_NETWORMS_MESSAGES_H
