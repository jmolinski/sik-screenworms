#include "../common/crc32.h"
#include "../common/messages.h"
#include <cassert>
#include <iostream>

int main(int argc, char *argv[]) {
    assert(argc > 1);
    int test_num = std::stoi(argv[1]);

    switch (test_num) {
        case 1: {
            const char *martinez = "martinez";
            uint32_t crc32v = utils::crc32(reinterpret_cast<const unsigned char *>(martinez), strlen(martinez));
            assert(crc32v == 3850936052);
            break;
        }
        case 2: {
            EventGameOver e{};
            e.encode(nullptr);
            [[maybe_unused]] auto e2 = EventGameOver(nullptr, 0);
            break;
        }
        case 3: {
            EventPlayerEliminated e(10);
            unsigned char buffer[10];
            e.encode(buffer);
            auto e2 = EventPlayerEliminated(buffer, 10);
            assert(e.playerNumber == e2.playerNumber);
            break;
        }
        case 4: {
            EventPixel e(100, 2137, 420);
            unsigned char buffer[10];
            e.encode(buffer);
            auto e2 = EventPixel(buffer, 10);
            assert(e.playerNumber == e2.playerNumber);
            assert(e.x == e2.x);
            assert(e.y == e2.y);
            break;
        }
        case 5: {
            EventNewGame e(100, 2137, {"janusz", "korwin", "mikke"});
            unsigned char buffer[100];
            uint32_t encodedSize = e.encode(buffer);
            auto e2 = EventNewGame(buffer, encodedSize);
            assert(e.maxy == e2.maxy);
            assert(e.maxx == e2.maxx);
            auto expectedPlayers =
                std::unordered_map<uint8_t, std::string>({{0, "janusz"}, {1, "korwin"}, {2, "mikke"}});
            auto actualPlayers = e.parsedPlayers();
            auto actualDecodedPlayers = e2.parsedPlayers();
            assert(actualPlayers == expectedPlayers);
            assert(actualDecodedPlayers == expectedPlayers);
            break;
        }
        case 6: {
            EventNewGame e(100, 2137, {"a", "b"});
            auto event1 = Event(420, EventType::newGame, e);
            uint32_t newGameEventSize = event1.getEncodedSize();

            EventPixel e2(10, 100, 100);
            auto event2 = Event(1, EventType::pixel, e2);
            uint32_t pixelEventSize = event2.getEncodedSize();

            ServerToClientMessage msg(0, {event1, event2});
            unsigned char buffer[1000];

            size_t encodedSize = msg.encode(buffer);
            assert(encodedSize == (newGameEventSize + pixelEventSize + 4));

            ServerToClientMessage msg2(buffer, encodedSize);
            assert(msg.gameId == msg2.gameId);
            assert(msg.events[0].eventType == msg2.events[0].eventType);

            break;
        }
        default: {
            throw std::runtime_error("unknown test case");
        }
    }
}
