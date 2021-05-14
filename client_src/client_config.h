#ifndef SIK_NETWORMS_CLIENT_CONFIG_H
#define SIK_NETWORMS_CLIENT_CONFIG_H

#include <string>
#include <unordered_map>

struct ClientConfig {
    std::string playerName;
    uint16_t serverPort;
    std::string guiServer;
    uint16_t guiPort;
    std::string gameServer;

    ClientConfig(const std::string &gameServer, std::unordered_map<char, std::string> params);
};

#endif // SIK_NETWORMS_CLIENT_CONFIG_H
