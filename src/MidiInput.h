#pragma once
#include <iostream>
#include <algorithm>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "Rosetta.h"
#include "RtMidi.h"

class MidiInput {
public:
    MidiInput(const std::string& _filename, int _MidiPort);
    virtual ~MidiInput();

    static void onMidi(double, std::vector<unsigned char>*, void*);
    Rosetta	threadData;

private:
    RtMidiIn* midiIn;
    void stringReplace(std::string*, char);
};
