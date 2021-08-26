#pragma once

#include <vector>

#include "File.h"
#include "Midi.h"

#include "midifile/include/MidiFile.h"

class MidiLog : public File, Midi {
public:
    MidiLog(void* _ctx, const std::string& _name);
    virtual ~MidiLog();

    void    close();
    void    trigger(const std::string& _track, unsigned char _status, size_t _channel, size_t _key, size_t _value);

private:
    smf::MidiFile               m_file;
    std::vector<std::string>    m_tracks;
    timespec                    m_epoc;

};