#include "Broadcaster.h"

#include "lo/lo.h"
#include "tools.h"

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

Broadcaster::Broadcaster() : 
    deviceName("default"),
    oscAddress("localhost"), oscPort("8000"), oscFolder("/"), osc(false),
    csvPre(""), csv(false),
    midiOut(nullptr)
{ }

Broadcaster::~Broadcaster() 
{ }

void sendValue(const std::string& _address, const std::string& _port, const std::string& _path, float _value) {
    lo_message m = lo_message_new();
    lo_message_add_float(m, _value);

    lo_address t = lo_address_new(_address.c_str(), _port.c_str());

    lo_send_message(t, _path.c_str(), m);
    lo_address_free(t);
    lo_message_free(m);
}

void sendValue(const std::string& _address, const std::string& _port, const std::string& _path, bool _value) {
    lo_message m = lo_message_new();
    lo_message_add_string(m, _value ? "on": "off");

    lo_address t = lo_address_new(_address.c_str(), _port.c_str());

    lo_send_message(t, _path.c_str(), m);
    lo_address_free(t);
    lo_message_free(m);
}

void sendValue(const std::string& _address, const std::string& _port, const std::string& _path, const std::string& _value) {
    lo_message m = lo_message_new();
    lo_message_add_string(m, _value.c_str());

    lo_address t = lo_address_new(_address.c_str(), _port.c_str());

    lo_send_message(t, _path.c_str(), m);
    lo_address_free(t);
    lo_message_free(m);
}

void Broadcaster::setLED(size_t _id, bool _value) {
    if (midiOut) {
        std::vector<unsigned char> msg;
        msg.push_back( 0xB0 );
        msg.push_back( _id );
        msg.push_back( _value? 127 : 0 );
        midiOut->sendMessage( &msg );
    }
}

bool Broadcaster::load(const std::string& _filename, const std::string& _setupname) {
    deviceName = _setupname;

    YAML::Node node = YAML::LoadFile(_filename);

    if (node[deviceName]) {
        data = node[deviceName];

        // Load OUT setup
        if (data["out"]) {

            // OSC
            if (data["out"]["osc"]) {
                osc = true;

                if (data["out"]["osc"]["address"]) 
                    oscAddress = data["out"]["osc"]["address"].as<std::string>();
                if (data["out"]["osc"]["port"]) 
                    oscPort = data["out"]["osc"]["port"].as<std::string>();
                if (data["out"]["osc"]["folder"])
                    oscFolder = data["out"]["osc"]["folder"].as<std::string>();
            }

            // CSV
            if (data["out"]["csv"]) {
                csv = true;

                if (data["out"]["csv"].IsMap())
                    if (data["out"]["csv"]["pre"])
                        csvPre = data["out"]["csv"]["pre"].as<std::string>();
            }
        }

        // Load default values
        if ( data["events"] ) {
            for ( size_t i = 0; i < data["events"].size(); i++ ) {
                if ( data["events"][i]["value"] )
                    values[i] = data["events"][i]["value"].as<float>();
                else if ( data["events"][i]["toggle"] )
                    toggles[i] = data["events"][i]["toggle"].as<bool>();

                broadcast(i);
            }
        }

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

bool Broadcaster::broadcast(std::vector<unsigned char>* _message) {
    std::string type;
    unsigned char channel;
    int bytes;

    extractHeader(_message, type, bytes, channel);

    size_t id = _message->at(1);
    std::string name = "unknown" + toString(id);

    if ( data["events"].IsNull() )
        return false;

    if ( type == "controller_change" &&
         id < data["events"].size() && 
         bytes == 2 ) {

        if ( data["events"][id] ) {
            if ( data["events"][id]["range"] ) {
                values[id] = map((float)_message->at(2), 0.0f, 127.0f, data["events"][id]["range"][0].as<float>(), data["events"][id]["range"][1].as<float>());
                return broadcast(id);
            }
            else if ( data["events"][id]["toggle"] ) {
                if ((int)_message->at(2) == 127) {
                    toggles[id] = !toggles[id];
                    return broadcast(id);
                }
            }
        }
    }

    // if (csv)
    //     std::cout << " x " << csvPre << name << " " << (int)_message->at(2) << " (" << type << ")" << std::endl;

    return false;
}

bool Broadcaster::broadcast(size_t _id) {

    if ( data["events"][_id]["name"] ) {
        std::string name = data["events"][_id]["name"].as<std::string>();

        if ( data["events"][_id]["toggle"] ) {
            setLED(_id, toggles[_id]);

            if (data["events"][_id][toggles[_id]? "on" : "off"]) {
                YAML::Node custom = data["events"][_id][toggles[_id]? "on" : "off"];

                if (custom) {
                    std::string msg = custom.as<std::string>();
                    stringReplace(msg, '_');
                    std::vector<std::string> el = split(msg, '_', true);

                    if (el.size() == 1) {
                        if (osc)
                            sendValue(oscAddress, oscPort, oscFolder + name, msg );

                        if (csv)
                            std::cout << oscFolder + name << "," << msg << std::endl;
                    }
                    else {
                        if (osc)
                            sendValue(oscAddress, oscPort, el[0], el[1] );

                        if (csv)
                            std::cout << el[0] << "," << el[1] << std::endl;
                    }

                    return true;
                }
            }
            else {

                if (osc)
                    sendValue(oscAddress, oscPort, oscFolder + name, toggles[_id]);

                if (csv)
                    std::cout << csvPre << name << "," << (toggles[_id]? "on" : "off") << std::endl;

                return true;

            }
        }
        else if ( data["events"][_id] ) {

            if (osc)
                sendValue(oscAddress, oscPort, oscFolder + name, values[_id]);

            if (csv)
                std::cout << csvPre << name << "," << values[_id] << std::endl;

            return true;
        }
    }

    return false;
}