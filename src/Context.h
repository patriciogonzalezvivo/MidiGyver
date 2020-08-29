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
#include "MidiDevice.h"
#include "ops/nodes.h"

enum DataType {
    TYPE_UNKNOWN,
    TYPE_BUTTON,
    TYPE_TOGGLE,
    TYPE_NUMBER,
    TYPE_VECTOR,
    TYPE_COLOR,
    TYPE_STRING
};

class Context {
public:

    Context();
    virtual ~Context();

    bool load(const std::string& _filename);
    bool save(const std::string& _filename);
    bool close();

    bool        updateDevice(const std::string& _device);

    bool        doKeyExist(const std::string& _device, size_t _key);
    YAML::Node  getKeyNode(const std::string& _device, size_t _key);
    std::string getKeyName(YAML::Node _keynode);
    DataType    getKeyDataType(YAML::Node _keynode);

    bool        shapeKeyValue(YAML::Node _keynode, const std::string& _device, const std::string& _type, size_t _key, float* _value);
    bool        mapKeyValue(YAML::Node _keynode, const std::string& _device, size_t _key, float _value);
    bool        updateKey(YAML::Node _keynode, const std::string& _device, size_t _key);
    bool        sendKeyValue(YAML::Node _keynode);

    std::vector<std::string>            targets;
    
    std::vector<std::string>            devicesNames;
    std::map<std::string, Device*>      devices;

    YAML::Node                          config;
    std::mutex                          configMutex;

protected:

    JSContext                           js;
    std::map<std::string, u_int32_t>    shapeFncs;
};
