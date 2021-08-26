#pragma once

#include <string>
#include <vector>

class Midi {
public:

    //status bytes
    static const unsigned char  CONTROLLER_CHANGE = 0xB0;
    static const unsigned char  NOTE_ON = 0x90;
    static const unsigned char  NOTE_OFF = 0x80;
    static const unsigned char  KEY_PRESSURE = 0xA0;
    static const unsigned char  PROGRAM_CHANGE = 0xC0;
    static const unsigned char  CHANNEL_PRESSURE = 0xD0;
    static const unsigned char  PITCH_BEND = 0xE0;
    static const unsigned char  SONG_POSITION = 0xF2;
    static const unsigned char  SONG_SELECT = 0xF3;
    static const unsigned char  TUNE_REQUEST = 0xF6;
    static const unsigned char  END_OF_SYSEX = 0xF7;
    static const unsigned char  TIMING_TICK = 0xF8;
    static const unsigned char  START_SONG = 0xFA;
    static const unsigned char  CONTINUE_SONG = 0xFB;
    static const unsigned char  STOP_SONG = 0xFC;
    static const unsigned char  ACTIVE_SENSING = 0xFE;
    static const unsigned char  SYSTEM_RESET = 0xFF;
    static const unsigned char  SYSTEM_EXCLUSIVE = 0xF0;

    static std::string          getStatusName(size_t i);
    static unsigned char        getStatusByte(size_t i);

    static std::string          statusByteToName(const unsigned char& _byte);
    static unsigned char        statusNameToByte(const std::string& _name);
    
    static void                 extractHeader(std::vector<unsigned char>* _message, unsigned char& _channel, unsigned char& _status, int& _bytes);

};
