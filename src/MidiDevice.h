#pragma once
#include <iostream>
#include <algorithm>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "Broadcaster.h"

class MidiDevice {
public:
    MidiDevice(const std::string& _filename, int _MidiPort);
    virtual ~MidiDevice();

    static void onMidi(double, std::vector<unsigned char>*, void*);
    Broadcaster	broadcaster;

private:
    RtMidiIn*   midiIn;
    RtMidiOut*  midiOut;
};
