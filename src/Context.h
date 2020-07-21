#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "ops/strings.h"
#include "yaml-cpp/yaml.h"

enum DataType {
    unknown,
    button,
    toggle,
    states,
    scalar,
    vector,
    color,
    script
};

class Context {
public:

    Context();
    virtual ~Context();

    bool load(const std::string& _filename);
    bool save(const std::string& _filename);

    bool updateKey(const std::string& _device, const std::string& _key);
    bool updateDevice(const std::string& _device);

    YAML::Node config;

    std::vector<std::string> devices;
};
