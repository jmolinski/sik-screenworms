#include "server.h"

Server::Server(ServerConfig parsedConfig) : config(parsedConfig), socket(parsedConfig.port) {
}

Server::~Server() {
}

void Server::run() {
}


