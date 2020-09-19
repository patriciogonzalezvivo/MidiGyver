#include "MidiDevice.h"

#include "ops/values.h"
#include "ops/strings.h"

#include "types/Color.h"
#include "types/Vector.h"

#include "Context.h"

// VIRTUAL PORT
MidiDevice::MidiDevice(void* _ctx, const std::string& _name) : midiIn(NULL), midiOut(NULL) {
    type = DEVICE_MIDI;
    ctx = _ctx;
    name = _name;
}

// REAL PORT
MidiDevice::MidiDevice(void* _ctx, const std::string& _name, size_t _midiPort) : midiIn(NULL), midiOut(NULL) {
    type = DEVICE_MIDI;
    ctx = _ctx;
    name = _name;
    midiPort = _midiPort;

    openInPort(_name, _midiPort);
    openOutPort(_name, _midiPort);
}

MidiDevice::~MidiDevice() {
    if (midiIn)
        delete midiIn;
    if (midiOut) 
        delete midiOut;
}

bool MidiDevice::openInPort(const std::string& _name, size_t _midiPort) {
    try {
        midiIn = new RtMidiIn(RtMidi::Api(0), "midigyver");
    } catch(RtMidiError &error) {
        error.printMessage();
        return false;
    }

    midiIn->openPort(_midiPort, _name);
    midiIn->setCallback(onMidi, this);
    midiIn->ignoreTypes(false, false, true);

    // midiName = midiIn->getPortName(_midiPort);
    // stringReplace(midiName, '_');
    return true;
}

bool MidiDevice::openOutPort(const std::string& _name, size_t _midiPort) {
    try {
        midiOut = new RtMidiOut(RtMidi::Api(0), "midigyver");
        midiOut->openPort(_midiPort, _name);
    }
    catch(RtMidiError &error) {
        error.printMessage();
        return false;
    }

    return true;
}

bool MidiDevice::openVirtualOutPort(const std::string& _name) {
    try {
        midiOut = new RtMidiOut(RtMidi::Api(0), "midigyver" );
        midiOut->openVirtualPort(_name);
    }
    catch(RtMidiError &error) {
        error.printMessage();
        return false;
    }

    return true;
}

void MidiDevice::send(const unsigned char _type) {
    std::vector<unsigned char> msg;
    msg.push_back( _type );
    midiOut->sendMessage( &msg );   
}

void MidiDevice::send(const unsigned char _type, size_t _key, size_t _value) {
    std::vector<unsigned char> msg;
    msg.push_back( _type );
    msg.push_back( _key );
    msg.push_back( _value );
    midiOut->sendMessage( &msg );   
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

    switch (status) {
        case MidiDevice::NOTE_OFF:
            _type = "note_off";
            _bytes = 2;
            break;

        case MidiDevice::NOTE_ON:
            _type = "note_on";
            _bytes = 2;
            break;

        case MidiDevice::KEY_PRESSURE:
            _type = "key_pressure";
            _bytes = 2;
            break;

        case MidiDevice::CONTROLLER_CHANGE:
            _type = "controller_change";
            _bytes = 2;
            break;

        case MidiDevice::PROGRAM_CHANGE:
            _type = "program_change";
            _bytes = 2;
            break;

        case MidiDevice::CHANNEL_PRESSURE:
            _type = "channel_pressure";
            _bytes = 2;
            break;

        case MidiDevice::PITCH_BEND:
            _type = "pitch_bend";
            _bytes = 2;
            break;

        case MidiDevice::SYSTEM_EXCLUSIVE:
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

        case MidiDevice::SONG_POSITION:
            _type = "song_position";
            _bytes = 2;
            break;

        case MidiDevice::SONG_SELECT:
            _type = "song_select";
            _bytes = 2;
            break;

        case MidiDevice::TUNE_REQUEST:
            _type = "tune_request";
            _bytes = 2;
            break;

        case MidiDevice::TIMING_TICK:
            _type = "timing_tick";
            _bytes = 0;
            break;

        case MidiDevice::START_SONG:
            _type = "start_song";
            _bytes = 0;
            break;

        case MidiDevice::CONTINUE_SONG:
            _type = "continue_song";
            _bytes = 0;
            break;

        case MidiDevice::STOP_SONG:
            _type = "stop_song";
            _bytes = 0;
            break;

        default:
            _type = "";
            _bytes = 0;
            break;
    }

    if (status == MidiDevice::NOTE_ON && _message->at(2) == 0) {
        _type = "note_off";
    }
}

std::vector<std::string> MidiDevice::getInputPorts() {
    std::vector<std::string> devices;

    RtMidiIn* midiIn = new RtMidiIn();
    unsigned int nPorts = midiIn->getPortCount();

    for(unsigned int i = 0; i < nPorts; i++) {
        std::string name = midiIn->getPortName(i);
        stringReplace(name, '_');
        devices.push_back( name );
    }

    delete midiIn;

    return devices;
}

void MidiDevice::onMidi(double _deltatime, std::vector<unsigned char>* _message, void* _userData) {
    unsigned int nBytes = 0;
    try {
        nBytes = _message->size();
    } 
    catch(RtMidiError &error) {
        error.printMessage();
        exit(EXIT_FAILURE);
    }

    MidiDevice *device = static_cast<MidiDevice*>(_userData);
    Context *context = static_cast<Context*>(device->ctx);

    std::string type;
    int bytes;
    unsigned char channel;
    extractHeader(_message, type, bytes, channel);

    size_t key = _message->at(1);
    float value = (float)_message->at(2);

    context->configMutex.lock();
    // std::cout << device->name << " Channel: " << channel << " Key: " << key << " Value:" << value << std::endl;
    if (context->doKeyExist(device->name, key)) {
        YAML::Node node = context->getKeyNode(device->name, key);
        if (context->shapeKeyValue(node, device->name, type, key, &value)) {
            context->mapKeyValue(node, device->name, key, value);
        }
    }
    context->configMutex.unlock();

}


