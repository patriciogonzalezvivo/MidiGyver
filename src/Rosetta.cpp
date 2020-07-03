#include "Rosetta.h"

#include <iostream>

//status bytes
const unsigned char NOTE_OFF = 0x80;
const unsigned char NOTE_ON = 0x90;
const unsigned char KEY_PRESSURE = 0xA0;
const unsigned char CONTROLLER_CHANGE = 0xB0;
const unsigned char PROGRAM_CHANGE = 0xC0;
const unsigned char CHANNEL_PRESSURE = 0xD0;
const unsigned char PITCH_BEND = 0xE0;
const unsigned char SYSTEM_EXCLUSIVE = 0xF0;
const unsigned char SONG_POSITION = 0xF2;
const unsigned char SONG_SELECT = 0xF3;
const unsigned char TUNE_REQUEST = 0xF6;
const unsigned char END_OF_SYSEX = 0xF7;
const unsigned char TIMING_TICK = 0xF8;
const unsigned char START_SONG = 0xFA;
const unsigned char CONTINUE_SONG = 0xFB;
const unsigned char STOP_SONG = 0xFC;
const unsigned char ACTIVE_SENSING = 0xFE;
const unsigned char SYSTEM_RESET = 0xFF;

Rosetta::Rosetta() {

}

Rosetta::~Rosetta() {

}

bool Rosetta::load(const std::string& _filename, const std::string& _setupname) {
    portName = _setupname;

    YAML::Node node = YAML::LoadFile(_filename);
    std::cout << portName << " " << node.size() << std::endl;

    if (node[portName]) {
        data = node[portName];
        return true;
    }

    return false;
}

void extractHeader(std::vector<unsigned char>* _message, std::string& _type, int& _bytes, unsigned char& _channel) {

    unsigned char status = 0;
    int j = 0;

    if ((_message->at(0) & 0xf0) != 0xf0) {
        _channel = _message->at(0) & 0x0f;
        status = _message->at(0) & 0xf0;
    }
    else {
        _channel = 0;
        status = _message->at(0);
    }

    switch(status) {
        case NOTE_OFF:
            _type = "note_off";
            _bytes = 2;
            break;

        case NOTE_ON:
            _type = "note_on";
            _bytes = 2;
            break;

        case KEY_PRESSURE:
            _type = "key_pressure";
            _bytes = 2;
            break;

        case CONTROLLER_CHANGE:
            _type = "controller_change";
            _bytes = 2;
            break;

        case PROGRAM_CHANGE:
            _type = "program_change";
            _bytes = 2;
            break;

        case CHANNEL_PRESSURE:
            _type = "channel_pressure";
            _bytes = 2;
            break;

        case PITCH_BEND:
            _type = "pitch_bend";
            _bytes = 2;
            break;

        case SYSTEM_EXCLUSIVE:
            if(_message->size() == 6) {
                unsigned int type = _message->at(4);
                if(type == 1) {
                    _type = "mmc_stop";
                } else if(type == 2) {
                    _type = "mmc_play";
                } else if(type == 4) {
                    _type = "mmc_fast_forward";
                } else if(type == 5) {
                    _type = "mmc_rewind";
                } else if(type == 6) {
                    _type = "mmc_record";
                } else if(type == 9) {
                    _type = "mmc_pause";
                }
            }
            _bytes = 0;
            break;

        case SONG_POSITION:
            _type = "song_position";
            _bytes = 2;
            break;

        case SONG_SELECT:
            _type = "song_select";
            _bytes = 2;
            break;

        case TUNE_REQUEST:
            _type = "tune_request";
            _bytes = 2;
            break;

        case TIMING_TICK:
            _type = "timing_tick";
            _bytes = 0;
            break;

        case START_SONG:
            _type = "start_song";
            _bytes = 0;
            break;

        case CONTINUE_SONG:
            _type = "continue_song";
            _bytes = 0;
            break;

        case STOP_SONG:
            _type = "stop_song";
            _bytes = 0;
            break;

        default:
            _type = "";
            _bytes = 0;
            break;
    }

    if(status == NOTE_ON && _message->at(2) == 0) {
        _type = "note_off";
    }
}

float map(float value, float inputMin, float inputMax, float outputMin, float outputMax) {
    float outVal = ((value - inputMin) / (inputMax - inputMin) * (outputMax - outputMin) + outputMin);
    return outVal;
}

float Rosetta::convert(std::vector<unsigned char>* _message) {
    std::string type;
    unsigned char channel;
    int bytes;

    extractHeader(_message, type, bytes, channel);

    // std::cout << " Port Name: " << portName << std::endl;
    // std::cout << "   Channel: " << (int)channel << std::endl;
    std::cout << "      Type: " << type << std::endl;

    size_t id = _message->at(1);
    float value = (float)_message->at(2);

    if (data[type].IsNull())
        return value;

    if (bytes == 2 && id < data[type].size() ) {
        if (!data[type][id].IsNull()) {
            if (data[type][id]["name"])
                std::cout << "      name: " << data[type][id]["name"].as<std::string>() << std::endl;

            if (data[type][id]["range"])
                value = map((float)_message->at(2), 0.0f, 127.0f, data[type][id]["range"][0].as<float>(), data[type][id]["range"][1].as<float>());
        }
    }
    else
        std::cout << "        id: " << (int)id << std::endl;

    std::cout << "     value: " << value << std::endl;

    return value;
}
