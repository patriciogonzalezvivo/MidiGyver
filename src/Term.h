#pragma once

#include <string>

enum TermType {
    PULSE,
    MIDI_FILE,
    MIDI_DEVICE,
    OSC_DEVICE
};

class Term {
public:

    virtual TermType    getType() const { return m_type; }
    virtual std::string getName() const { return m_name; }
    virtual bool        close() = 0;

protected:

    std::string     m_name;
    TermType        m_type;
    void*           m_ctx;
};
