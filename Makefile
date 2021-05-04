CXX=g++
CPPFLAGS= -std=c++17 -O2 -Wall -Wextra -Werror -Wpedantic
LDFLAGS=

CLIENT_SRCS= client_src/main.cpp utils/getopt_wrapper.cpp utils/utils.cpp
SERVER_SRCS= server_src/main.cpp utils/getopt_wrapper.cpp utils/utils.cpp
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
