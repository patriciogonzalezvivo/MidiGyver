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
#include "MidiLog.h"
#include "ops/nodes.h"
#include "ops/source.h"

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

    bool    load(const std::string& _filename);
    Term*   loadMidiDeviceIn(const std::string& _devicesName, YAML::Node _node);
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
    std::vector<Target> getTargetsForNode(YAML::Node _node);

    bool        processEvent(YAML::Node _node, const std::string& _term, unsigned char _status, size_t _channel, size_t _key, float _value, bool _statusOnly);
    bool        shapeValue(YAML::Node _node, const std::string& _term, unsigned char _status, size_t _channel, size_t _key, float* _value, bool _statusOnly);
    bool        mapValue(YAML::Node _node, const std::string& _term, unsigned char _status, size_t _channel, size_t _key, float _value);

    bool        updateNode(YAML::Node _node, const std::string& _term, unsigned char _status, size_t _channel, size_t _key);

    bool        feedback(const std::string& _term, unsigned char _status, size_t _channel, size_t _key, size_t _value);

    std::vector<Source>                 sources;
    std::map<std::string, Term*>        sourcesTerm;

    std::vector<Target>                 targets;
    std::map<std::string, Term*>        targetsTerm;

    YAML::Node                          config;
    std::mutex                          configMutex;
    int                                 tickDuration; // Milli Secs
    bool                                safe;
protected:

    JSContext                           js;
    std::map<std::string, size_t>       shapeFncs;
};
