#pragma once

#include <functional>
#include "yaml-cpp/yaml.h"

#include "Device.h"

class Pulse : public Device {
public:

    Pulse(void* _ctx, size_t _index);
    virtual ~Pulse();

    void    start(size_t _milliSec);
    void    stop();

    size_t  index;

private:

    bool    clear = false;
};