#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "rtmidi/RtMidi.h"

#include "ops/nodes.h"

enum DataType {
    unknown_type,
    button_type,
    toggle_type,
    state_type,
    scalar_type,
    vector_type,
    color_type
};

class Context {
public:

    Context();
    virtual ~Context();

    bool load(const std::string& _filename);
    bool save(const std::string& _filename);

    bool        updateDevice(const std::string& _device);

    bool        doKeyExist(const std::string& _device, size_t _key);
    YAML::Node  getKeyNode(const std::string& _device, size_t _key);
    std::string getKeyName(const std::string& _device, size_t _key);
    DataType    getKeyDataType(const std::string& _device, size_t _key);

    bool        shapeKeyValue(const std::string& _device, size_t _key, float* _value);
    bool        mapKeyValue(const std::string& _device, size_t _key, float _value);
    bool        updateKey(const std::string& _device, size_t _key);
    bool        sendKeyValue(const std::string& _device, size_t _key);

    std::vector<std::string>            targets;
    std::vector<std::string>            devices;
    std::map<std::string, RtMidiOut*>   devicesOut;

    YAML::Node                          config;

protected:

    JSContext                           js;
    std::map<std::string, u_int32_t>    shapeFncs;
};
