#pragma once

#include <string>

enum TermType {
    PULSE,
    MIDI_FILE,
    MIDI_DEVICE
};

class Term {
public:

    std::string     name;
    TermType        type;

protected:

    void*           ctx;
};
