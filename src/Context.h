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
#include "MidiLog.h"
#include "MidiDevice.h"
#include "OscDevice.h"

#include "ops/nodes.h"
#include "ops/address.h"
#include "types/DataType.h"

class Context {
public:

    Context();
    virtual ~Context();

    bool    load(const std::string& _filename);
    Term*   loadMidiDeviceIn(const std::string& _devicesName, YAML::Node _node);
    Term*   loadOscDeviceIn(const std::string& _devicesName, size_t _port, YAML::Node _node);
    void    updateDevice(const std::string& _name);

    bool    save(const std::string& _filename);
    bool    close();

    // STATUS ONLY EVENTS
    bool        doStatusExist(const std::string& _term, unsigned char _status);
    YAML::Node  getStatusNode(const std::string& _term, unsigned char _status);

    // KEYS EVENTS 
    bool        doKeyExist(const std::string& _term, size_t _channel, size_t _key);
    YAML::Node  getKeyNode(const std::string& _term, size_t _channel, size_t _key);

    // Common Proces
    DataType    getKeyDataType(YAML::Node _node);
    AddressList getTargetsForNode(YAML::Node _node);

    bool        processEvent(YAML::Node _node, const std::string& _term, unsigned char _status, size_t _channel, size_t _key, float _value, bool _statusOnly);
    bool        shapeValue(YAML::Node _node, const std::string& _term, unsigned char _status, size_t _channel, size_t _key, float* _value, bool _statusOnly);
    bool        mapValue(YAML::Node _node, const std::string& _term, unsigned char _status, size_t _channel, size_t _key, float _value);

    bool        updateNode(YAML::Node _node, const std::string& _term, unsigned char _status, size_t _channel, size_t _key);

    bool        feedback(const std::string& _term, unsigned char _status, size_t _channel, size_t _key, size_t _value);

    AddressList                     sources;
    std::map<std::string, Term*>    sourcesTerm;

    AddressList                     targets;
    std::map<std::string, Term*>    targetsTerm;

    YAML::Node                      config;
    std::mutex                      configMutex;
    int                             tickDuration; // Milli Secs
    bool                            safe;
protected:

    JSContext                       js;
    std::map<std::string, size_t>   shapeFncs;
};
