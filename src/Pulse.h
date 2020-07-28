#pragma once

#include <functional>

#include "yaml-cpp/yaml.h"

class Pulse {
public:

    Pulse(void* _ctx, size_t _index);
    virtual ~Pulse();

    void start(size_t _interval);

    void stop();

    std::string name;
    size_t      index;

private:
    void*       ctx;

    bool clear = false;
};