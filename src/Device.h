#pragma once

#include <map>

enum DeviceType {
    DEVICE_PULSE,
    DEVICE_MIDI
};

class Device {
public:

    std::string                 name;
    DeviceType                  type;
    std::map<size_t, size_t>    keyMap;

protected:
    void*       ctx;
};
