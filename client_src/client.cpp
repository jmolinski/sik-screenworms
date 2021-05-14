#include "client.h"

Client::Client(ClientConfig config) : config(config), serverSocket(config.serverPort) {
    // TODO pass server addr
}

Client::~Client() {
}

void Client::run() {
}
