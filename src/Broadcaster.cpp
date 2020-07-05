#include "Broadcaster.h"

#include <iostream>
#include <cmath> 

#include "lo/lo.h"
#include "tools.h"

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
    csv(false),
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

            // CSV
            if (data["out"]["csv"])
                csv = true;

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
        }

        // Load default values
        if ( data["events"] )
            for ( size_t i = 0; i < data["events"].size(); i++ )
                if (data["events"][i]["value"])
                    broadcast(i);

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
                if (type == 1)
                    _type = "mmc_stop";
                else if(type == 2)
                    _type = "mmc_play";
                else if(type == 4)
                    _type = "mmc_fast_forward";
                else if(type == 5)
                    _type = "mmc_rewind";
                else if(type == 6)
                    _type = "mmc_record";
                else if(type == 9)
                    _type = "mmc_pause";
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
            std::string type = "none";

            if ( data["events"][id]["type"] )
                type =  data["events"][id]["type"].as<std::string>();

            if ( type == "button" ) {
                data["events"][id]["value"] = (int)_message->at(2) == 127;
                return broadcast(id);
            }
            else if ( type == "switch" ) {
                
                if ((int)_message->at(2) == 127) {
                    bool value = data["events"][id]["value"].as<bool>();

                    data["events"][id]["value"] = !value;

                    return broadcast(id);
                }

            }
            else if ( type == "scalar" ) {
                float value = (float)_message->at(2);

                if ( data["events"][id]["map"] )
                    value = map(value, 0.0f, 127.0f, data["events"][id]["map"][0].as<float>(), data["events"][id]["map"][1].as<float>());

                data["events"][id]["value"] = value;

                return broadcast(id);
            }
            else if ( type == "states" ) {
                int value = (int)_message->at(2);
                std::string value_str = toString(value);

                if ( data["events"][id]["map"] ) {
                    if ( data["events"][id]["map"].IsSequence() ) {
                        float total = data["events"][id]["map"].size();

                        if (value == 127.0f) {
                            value_str = data["events"][id]["map"][total-1].as<std::string>();
                        } 
                        else {
                            size_t index = (value / 127.0f) * (data["events"][id]["map"].size());
                            value_str = data["events"][id]["map"][index].as<std::string>();
                        } 
                    }
                }
                
                data["events"][id]["value"] = value_str;
                return broadcast(id);
            }
            else if ( type == "lerp" ) {
                float value = (float)_message->at(2) / 127.0f;

                if ( data["events"][id]["map"] ) {
                    if ( data["events"][id]["map"].IsSequence() ) {
                        if (data["events"][id]["map"].size() > 1) {
                            float total = data["events"][id]["map"].size() - 1;

                            if (value == 127.0f) {
                                value = data["events"][id]["map"][total-1].as<float>();
                            } 
                            else {
                                size_t i_low = value * total;
                                size_t i_high = std::min(i_low + 1, size_t(total));
                                float pct = (value * total) - (float)i_low;
                                value = lerp(   data["events"][id]["map"][i_low].as<float>(),
                                                data["events"][id]["map"][i_high].as<float>(),
                                                pct );
                            }
                        }

                    }
                }
                
                data["events"][id]["value"] = value;
                return broadcast(id);
            }
            
        }
    }

    return false;
}

bool Broadcaster::broadcast(size_t _id) {
    std::string name = "unknown" + toString(_id);
    std::string type = "none";
    YAML::Node  value = data["events"][_id]["value"];

    if ( data["events"][_id]["value"].IsNull() )
        return false;

    if ( data["events"][_id]["name"] )
        name = data["events"][_id]["name"].as<std::string>();

    if ( data["events"][_id]["type"] )
        type =  data["events"][_id]["type"].as<std::string>();

    if ( type == "switch" || type == "button" ) {
        std::string value_str = (value.as<bool>()) ? "on" : "off";
        setLED(_id, value.as<bool>() );

        if (data["events"][_id]["map"]) {

            if (data["events"][_id]["map"][value_str]) {
                YAML::Node custom = data["events"][_id]["map"][value_str];

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

        }
        else {

            if (osc)
                sendValue(oscAddress, oscPort, oscFolder + name, value_str);

            if (csv)
                std::cout << name << "," << (value_str) << std::endl;

            return true;
        }
    }

    else if ( type == "scalar" || type == "lerp" ) {
        if (osc)
            sendValue(oscAddress, oscPort, oscFolder + name, value.as<float>());

        if (csv)
            std::cout << name << "," << value.as<float>() << std::endl;

        return true;
    }

    else if ( type == "states" ) {
        if (osc)
            sendValue(oscAddress, oscPort, oscFolder + name, value.as<std::string>());

        if (csv)
            std::cout << name << "," << value.as<std::string>() << std::endl;

        return true;
    }

    return false;
}