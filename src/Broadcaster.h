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

    void setLED(size_t _id, bool _value);

private:
    std::string deviceName;

    std::string oscAddress;
    std::string oscPort;
    std::string oscFolder;
    bool        osc;

    std::string csvPre;
    bool        csv;

    std::map<size_t, bool>  toggles;
    std::map<size_t, float> values;

    YAML::Node  data;
    RtMidiOut*  midiOut;
};
