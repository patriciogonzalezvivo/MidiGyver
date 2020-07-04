#pragma once

#include <string>

#include "yaml-cpp/yaml.h"

class Broadcaster {
public:
    Broadcaster();
    virtual ~Broadcaster();

    bool load(const std::string& _filename, const std::string& _setupname);

    bool broadcast(std::vector<unsigned char>* _message);

    std::string portName;

private:
    YAML::Node data;

};
