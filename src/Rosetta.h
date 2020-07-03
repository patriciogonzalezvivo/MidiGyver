#pragma once

#include <string>

#include "yaml-cpp/yaml.h"

class Rosetta {
public:
    Rosetta();
    virtual ~Rosetta();

    bool load(const std::string& _filename, const std::string& _setupname);

    float convert(std::vector<unsigned char>* _message);

    std::string portName;

private:
    YAML::Node data;

};
