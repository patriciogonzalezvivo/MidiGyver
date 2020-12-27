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

    //status bytes
    static const unsigned char NOTE_OFF = 0x80;
    static const unsigned char NOTE_ON = 0x90;
    static const unsigned char KEY_PRESSURE = 0xA0;
    static const unsigned char CONTROLLER_CHANGE = 0xB0;
    static const unsigned char PROGRAM_CHANGE = 0xC0;
    static const unsigned char CHANNEL_PRESSURE = 0xD0;
    static const unsigned char PITCH_BEND = 0xE0;
    static const unsigned char SYSTEM_EXCLUSIVE = 0xF0;
    static const unsigned char SONG_POSITION = 0xF2;
    static const unsigned char SONG_SELECT = 0xF3;
    static const unsigned char TUNE_REQUEST = 0xF6;
    static const unsigned char END_OF_SYSEX = 0xF7;
    static const unsigned char TIMING_TICK = 0xF8;
    static const unsigned char START_SONG = 0xFA;
    static const unsigned char CONTINUE_SONG = 0xFB;
    static const unsigned char STOP_SONG = 0xFC;
    static const unsigned char ACTIVE_SENSING = 0xFE;
    static const unsigned char SYSTEM_RESET = 0xFF;

    MidiDevice(void* _ctx, const std::string& _name);
    MidiDevice(void* _ctx, const std::string& _name, size_t _midiPort);
    virtual ~MidiDevice();

    bool    openInPort(const std::string& _name, size_t _midiPort);
    bool    openOutPort(const std::string& _name, size_t _midiPort);
    bool    openVirtualOutPort(const std::string& _name);

    static std::vector<std::string> getInPorts();
    static std::vector<std::string> getOutPorts();

    static void onMidi(double, std::vector<unsigned char>*, void*);

    void        send(const unsigned char _type);
    void        send(size_t _key, size_t _value);
    void        send(const unsigned char _type, size_t _key, size_t _value);
    void        send(const unsigned char _type, unsigned char _channel, size_t _key, size_t _value);

    size_t      midiPort;
    
    size_t          defaultOutChannel;
    unsigned char   defaultOutType;

protected:
    RtMidiIn*   midiIn;
    RtMidiOut*  midiOut;
};
