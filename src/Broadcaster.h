#pragma once

#include <string>
#include <map>

#include "RtMidi.h"
#include "yaml-cpp/yaml.h"

class Broadcaster {
public:
    Broadcaster();
    virtual ~Broadcaster();

    bool load(const std::string& _filename, const std::string& _setupname);
    void setCallbackPort(RtMidiOut* _midiout) { midiOut = _midiout; };

    bool broadcast(std::vector<unsigned char>* _message);
    bool broadcast(size_t _id);

    void setLED(size_t _id, bool _value);

    YAML::Node  data;
    std::string deviceName;

private:
    std::string oscAddress;
    std::string oscPort;
    std::string oscFolder;
    bool        osc;
    bool        csv;

    RtMidiOut*  midiOut;
};
