#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <mutex>

#include "rtmidi/RtMidi.h"

#include "Pulse.h"
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

struct DeviceData {
    RtMidiOut*                  out;
    std::map<size_t, size_t>    keyMap;
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
    std::string getKeyName(YAML::Node _keynode);
    DataType    getKeyDataType(YAML::Node _keynode);

    bool        shapeKeyValue(YAML::Node _keynode, const std::string& _device, size_t _key, float* _value);
    bool        mapKeyValue(YAML::Node _keynode, const std::string& _device, size_t _key, float _value);
    bool        updateKey(YAML::Node _keynode, const std::string& _device, size_t _key);
    bool        sendKeyValue(YAML::Node _keynode);

    std::vector<std::string>            targets;
    std::vector<std::string>            devices;
    std::map<std::string, DeviceData>   devicesData;
    std::vector<Pulse*>                 pulses;

    YAML::Node                          config;
    std::mutex                          configMutex;

protected:

    JSContext                           js;
    std::map<std::string, u_int32_t>    shapeFncs;
};
