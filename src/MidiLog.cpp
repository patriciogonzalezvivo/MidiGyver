#include "MidiLog.h"

#include <sstream>

#include "ops/times.h"

MidiLog::MidiLog(void* _ctx, const std::string& _name) {
    type = FILE_MIDI;
    ctx = _ctx;
    name = _name;

    m_file.setMillisecondTicks();
    std::cout << "Saving midi data to: " << _name << std::endl;

    clock_gettime(CLOCK_MONOTONIC, &m_epoc);
}

MidiLog::~MidiLog() {
    close();
}

void MidiLog::close() {
    m_file.sortTracks();
    m_file.write( name );
}

void MidiLog::trigger(const std::string& _track, unsigned char _status, size_t _channel, size_t _key, size_t _value) {
    // Search for the track number
    size_t track = m_tracks.size();
    for (size_t i = 0; i < track; i++)
        if (_track == m_tracks[i]) 
            track = i;
    
    int ms = getTimeMs(m_epoc);

    // If it didn't find it, create one
    if (track == m_tracks.size()) {
        m_tracks.push_back(_track);
        m_file.addTrack();
        m_file.addTrackName(track, ms, _track);
    }

    if (_status == Midi::NOTE_ON)
        m_file.addNoteOn(track, ms, _channel, _key, _value);
    else if (_status == Midi::NOTE_OFF)
        m_file.addNoteOff(track, ms, _channel, _key, _value);
    else if (_status == Midi::CONTROLLER_CHANGE )
        m_file.addController(track, ms, _channel, _key, _value);
    
    m_file.write( name );
}