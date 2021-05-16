#include "messages.h"
#include <cstring>
#include <endian.h>

template <typename T>
static inline T binaryToNum(const unsigned char *buff) {
    T num = 0;
    for (size_t i = 0; i < sizeof(T); i++) {
        num <<= 8;
        num |= buff[i];
    }
    return num;
}

template <typename T>
static inline void numToBinary(T num, unsigned char *buff) {
    auto *numBPtr = reinterpret_cast<unsigned char *>(&num);
    for (size_t i = 0; i < sizeof(T); i++) {
        buff[i] = numBPtr[i];
    }
}

ClientToServerMessage::ClientToServerMessage(const unsigned char *buffer, size_t size) {
    if (size < min_struct_size || size > max_struct_size) {
        throw EncoderDecoderError();
    }

    sessionId = be64toh(binaryToNum<uint64_t>(buffer));
    turnDirection = static_cast<TurnDirection>(buffer[8]);
    nextExpectedEventNo = be32toh(binaryToNum<uint32_t>(buffer + 9));
    playerNameSize = size - min_struct_size;
    memset(playerName, 0, sizeof playerName);
    memcpy(playerName, buffer + min_struct_size, playerNameSize);
    if (buffer[8] > static_cast<uint8_t>(TurnDirection::left) || !utils::isValidPlayerName(playerName)) {
        throw EncoderDecoderError();
    }
}

size_t ClientToServerMessage::encode(unsigned char *buffer) {
    numToBinary(htobe64(sessionId), buffer);
    numToBinary(turnDirection, buffer);
    numToBinary(htobe32(nextExpectedEventNo), buffer);
    memcpy(buffer + min_struct_size, playerName, playerNameSize);
    return min_struct_size + playerNameSize;
}
