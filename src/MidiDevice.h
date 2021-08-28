#pragma once

#include <iostream>
#include <algorithm>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "rtmidi/RtMidi.h"

#include "Device.h"
#include "Midi.h"
#include "ops/nodes.h"

class MidiDevice : public Device, Midi {
public:

    static std::vector<std::string> getInPorts();
    static std::vector<std::string> getOutPorts();
    
    MidiDevice(void* _ctx, const std::string& _name);
    MidiDevice(void* _ctx, const std::string& _name, size_t _midiPort);
    virtual ~MidiDevice();

    bool            openInPort(const std::string& _name, size_t _midiPort);
    bool            openOutPort(const std::string& _name, size_t _midiPort);
    bool            openVirtualOutPort(const std::string& _name);
    bool            close();

    static void     onMidi(double, std::vector<unsigned char>*, void*);

    void            trigger(unsigned char _status, unsigned char _channel);
    void            trigger(unsigned char _status, unsigned char _channel, size_t _key, size_t _value);

    YAML::Node      node;
    size_t          midiPort;
    size_t          defaultOutChannel;
    size_t          tickCounter;
    unsigned char   defaultOutStatus;

protected:
    RtMidiIn*       midiIn;
    RtMidiOut*      midiOut;
};

