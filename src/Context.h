#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "ops/nodes.h"

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

    std::vector<std::string>            devices;
    std::map<std::string, OscTarget>    oscTargets;

    YAML::Node                          config;
    JSContext                           js;
};
