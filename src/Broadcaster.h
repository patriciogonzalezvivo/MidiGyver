#pragma once

#include <string>

#include "yaml-cpp/yaml.h"

class Broadcaster {
public:
    Broadcaster();
    virtual ~Broadcaster();

    bool load(const std::string& _filename, const std::string& _setupname);

    bool broadcast(std::vector<unsigned char>* _message);

private:
    std::string deviceName;

    std::string oscAddress;
    std::string oscPort;
    std::string oscFolder;
    bool        osc;

    std::string csvPre;
    bool        csv;

    YAML::Node data;

};
