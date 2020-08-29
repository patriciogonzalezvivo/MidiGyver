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

class MidiDevice : public Device {
public:
    MidiDevice(void* _ctx, const std::string& _name, size_t _midiPort);
    virtual ~MidiDevice();

    static std::vector<std::string> getInputPorts();
    static void onMidi(double, std::vector<unsigned char>*, void*);

    void        send_CC(size_t _key, size_t _value);

    size_t      midiPort;

protected:
    RtMidiIn*   midiIn;
    RtMidiOut*  midiOut;
};
