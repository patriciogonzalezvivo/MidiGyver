#pragma once
#include <iostream>
#include <algorithm>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "RtMidi.h"
#include "Broadcaster.h"

class MidiInput {
public:
    MidiInput(const std::string& _filename, int _MidiPort);
    virtual ~MidiInput();

    static void onMidi(double, std::vector<unsigned char>*, void*);
    Broadcaster	broadcaster;

private:
    RtMidiIn* midiIn;
    void stringReplace(std::string*, char);
};
