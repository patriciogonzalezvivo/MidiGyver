#pragma once

#include <iostream>
#include <algorithm>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "Context.h"

#include "rtmidi/RtMidi.h"

class MidiDevice {
public:
    MidiDevice(Context* _ctx, const std::string& _deviceName, size_t _midiPort);
    virtual ~MidiDevice();

    static void onMidi(double, std::vector<unsigned char>*, void*);
    
    bool parseMessage(size_t _key, size_t _value);
    void setLED(size_t _key, bool _value);

    std::string deviceName;

    std::string midiName;
    size_t      midiPort;

// private:
    Context*    ctx;
    RtMidiIn*   midiIn;
    RtMidiOut*  midiOut;
};
