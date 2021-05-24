CXX=g++
CPPFLAGS= -std=c++17 -O2 -Wall -Wextra
LDFLAGS=

COMMON_SRCS= common/crc32.cpp common/fingerprint.cpp common/getopt_wrapper.cpp common/messages.cpp common/misc.cpp common/sockets.cpp common/time_utils.cpp
CLIENT_SRCS= client_src/main.cpp client_src/client.cpp client_src/client_config.cpp $(COMMON_SRCS)
SERVER_SRCS= server_src/main.cpp server_src/server.cpp server_src/server_config.cpp server_src/game.cpp $(COMMON_SRCS)
CLIENT_OBJS=$(subst .cpp,.o,$(CLIENT_SRCS))
SERVER_OBJS=$(subst .cpp,.o,$(SERVER_SRCS))

all: screen-worms-server screen-worms-client

screen-worms-server: $(SERVER_OBJS)
	$(CXX) -o screen-worms-server $(SERVER_OBJS) $(LDFLAGS)

screen-worms-client: $(CLIENT_OBJS)
	$(CXX) -o screen-worms-client $(CLIENT_OBJS) $(LDFLAGS)

depend: .depend

.depend: $(CLIENT_SRCS) $(SERVER_SRCS)
	rm -rf ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	rm -rf $(CLIENT_OBJS) $(SERVER_OBJS) .depend screen-worms-server screen-worms-client

include .depend
