#include "client.h"

#include <utility>

Client::Client(ClientConfig conf) : config(std::move(conf)){
    // TODO pass server addr
}

Client::~Client() {
}

void Client::run() {
}
