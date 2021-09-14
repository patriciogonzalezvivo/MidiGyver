#include "Midi.h"


unsigned char statusByte[18] = { 
    Midi::CONTROLLER_CHANGE,
    Midi::NOTE_ON,
    Midi::NOTE_OFF,
    Midi::KEY_PRESSURE,
    Midi::PROGRAM_CHANGE,
    Midi::CHANNEL_PRESSURE,
    Midi::PITCH_BEND,
    Midi::SONG_POSITION,
    Midi::SONG_SELECT,
    Midi::TUNE_REQUEST,
    Midi::TIMING_TICK,
    Midi::START_SONG,
    Midi::CONTINUE_SONG,
    Midi::STOP_SONG,
    Midi::ACTIVE_SENSING,
    Midi::SYSTEM_RESET,
    Midi::SYSTEM_EXCLUSIVE,
    Midi::END_OF_SYSEX
};

std::string statusNames[18] = { 
    "CONTROLLER_CHANGE",
    "NOTE_ON", 
    "NOTE_OFF", 
    "KEY_PRESSURE", 
    "PROGRAM_CHANGE",
    "CHANNEL_PRESSURE",
    "PITCH_BEND",
    "SONG_POSITION",
    "SONG_SELECT",
    "TUNE_REQUEST",
    "TIMING_TICK",
    "START_SONG",
    "CONTINUE_SONG",
    "STOP_SONG",
    "ACTIVE_SENSING",
    "SYSTEM_RESET",
    "SYSTEM_EXCLUSIVE",
    "END_OF_SYSEX"
};

unsigned char Midi::getStatusByte(size_t i) { return statusByte[i]; }

std::string Midi::getStatusName(size_t i) { return statusNames[i]; }

std::string Midi::statusByteToName(const unsigned char& _type) {
    for (int i = 0; i < 18; i++ ) {
        if (statusByte[i] == _type)
            return statusNames[i];
    }
    return "NONE";
}

unsigned char Midi::statusNameToByte(const std::string& _name) {
    for (int i = 0; i < 18; i++ ) {
        if (statusNames[i] == _name)
            return statusByte[i];
    }
    return 0;
}


void Midi::extractHeader(std::vector<unsigned char>* _message, unsigned char& _channel, unsigned char& _status, int& _bytes) {
    int j = 0;

    if ((_message->at(0) & 0xf0) != 0xf0) {
        _channel = _message->at(0) & 0x0f;
        _channel += 1;
        _status = _message->at(0) & 0xf0;
    }
    else {
        _channel = 0;
        _status = _message->at(0);
    }

    switch (_status) {
        case Midi::CONTROLLER_CHANGE:
            _bytes = 2;
            break;

        case Midi::NOTE_ON:
            _bytes = 2;
            break;

        case Midi::NOTE_OFF:
            _bytes = 2;
            break;

        case Midi::KEY_PRESSURE:
            _bytes = 2;
            break;

        case Midi::PROGRAM_CHANGE:
            _bytes = 1;
            break;

        case Midi::CHANNEL_PRESSURE:
            _bytes = 1;
            break;

        case Midi::PITCH_BEND:
            _bytes = 2;
            break;

        case Midi::SONG_POSITION:
            _bytes = 2;
            break;

        case Midi::SONG_SELECT:
            _bytes = 2;
            break;

        case Midi::TUNE_REQUEST:
            _bytes = 2;
            break;

        case Midi::TIMING_TICK:
            _bytes = 0;
            break;

        case Midi::START_SONG:
            _bytes = 0;
            break;

        case Midi::CONTINUE_SONG:
            _bytes = 0;
            break;

        case Midi::STOP_SONG:
            _bytes = 0;
            break;

        case Midi::SYSTEM_EXCLUSIVE:
            // if(_message->size() == 6) {
            //     unsigned int type = _message->at(4);
            //     if (type == 1)
            //         _type = "mmc_stop";
            //     else if(type == 2)
            //         _type = "mmc_play";
            //     else if(type == 4)
            //         _type = "mmc_fast_forward";
            //     else if(type == 5)
            //         _type = "mmc_rewind";
            //     else if(type == 6)
            //         _type = "mmc_record";
            //     else if(type == 9)
            //         _type = "mmc_pause";
            // }
            _bytes = 0;
            break;

        default:
            _bytes = 0;
            break;
    }

    if (_status == Midi::NOTE_ON && 
        _message->at(2) == 0) {
        _status = Midi::NOTE_OFF;
    }
}
