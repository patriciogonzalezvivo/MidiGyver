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
    
    TYPE_STRING,

    TYPE_MIDI_NOTE,
    TYPE_MIDI_CONTROLLER_CHANGE,
    TYPE_MIDI_TIMING_TICK
};

class Context {
public:

    Context();
    virtual ~Context();

    bool load(const std::string& _filename);
    bool save(const std::string& _filename);
    bool close();

    bool        updateDevice(const std::string& _device);

    bool        doKeyExist(const std::string& _device, size_t _channel, size_t _key);
    YAML::Node  getKeyNode(const std::string& _device, size_t _channel, size_t _key);
    std::string getKeyName(YAML::Node _keynode);
    DataType    getKeyDataType(YAML::Node _keynode);
    std::vector<Target> getTargetsForNode(YAML::Node _keynode);

    bool        processKey(YAML::Node _keynode, const std::string& _device, unsigned char _status, size_t _channel, size_t _key, float _value);
    bool        shapeKeyValue(YAML::Node _keynode, const std::string& _device, unsigned char _status, size_t _channel, size_t _key, float* _value);
    bool        mapKeyValue(YAML::Node _keynode, const std::string& _device, unsigned char _status, size_t _channel, size_t _key, float _value);
    
    bool        updateKey(YAML::Node _keynode, const std::string& _device, unsigned char _status, size_t _channel, size_t _key);

    bool        feedback(const std::string& _device, unsigned char _status, size_t _channel, size_t _key, size_t _value);

    std::vector<std::string>            listenDevicesNames;
    std::map<std::string, Device*>      listenDevices;

    std::vector<Target>                 targets;
    std::vector<std::string>            targetsDevicesNames;
    std::map<std::string, Device*>      targetsDevices;

    YAML::Node                          config;
    std::mutex                          configMutex;
protected:

    JSContext                           js;
    std::map<std::string, size_t>       shapeFncs;
    std::map<std::string, size_t>       namingFncs;
};
