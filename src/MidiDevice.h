#pragma once

#include <iostream>
#include <algorithm>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "Context.h"



class MidiDevice {
public:
    MidiDevice(Context* _ctx, const std::string& _deviceName, size_t _midiPort);
    virtual ~MidiDevice();

    static void onMidi(double, std::vector<unsigned char>*, void*);

    std::string deviceName;
    size_t      midiPort;

    // std::string midiName;

// private:
    Context*    ctx;
    RtMidiIn*   midiIn;
};
