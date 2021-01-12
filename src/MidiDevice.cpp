#include "MidiDevice.h"

#include "ops/values.h"
#include "ops/strings.h"

#include "types/Color.h"
#include "types/Vector.h"

#include "Context.h"

#include <thread>
#include <chrono>

unsigned char statusByte[18] = { 
    MidiDevice::CONTROLLER_CHANGE,
    MidiDevice::NOTE_ON,
    MidiDevice::NOTE_OFF,
    MidiDevice::KEY_PRESSURE,
    MidiDevice::PROGRAM_CHANGE,
    MidiDevice::CHANNEL_PRESSURE,
    MidiDevice::PITCH_BEND,
    MidiDevice::SONG_POSITION,
    MidiDevice::SONG_SELECT,
    MidiDevice::TUNE_REQUEST,
    MidiDevice::TIMING_TICK,
    MidiDevice::START_SONG,
    MidiDevice::CONTINUE_SONG,
    MidiDevice::STOP_SONG,
    MidiDevice::ACTIVE_SENSING,
    MidiDevice::SYSTEM_RESET,
    MidiDevice::SYSTEM_EXCLUSIVE,
    MidiDevice::END_OF_SYSEX
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

unsigned char MidiDevice::getStatusByte(size_t i) { return statusByte[i]; }
std::string MidiDevice::getStatusName(size_t i) { return statusNames[i]; }

std::string MidiDevice::statusByteToName(const unsigned char& _type) {
    for (int i = 0; i < 18; i++ ) {
        if (statusByte[i] == _type)
            return statusNames[i];
    }
    return "NONE";
}

unsigned char MidiDevice::statusNameToByte(const std::string& _name) {
    for (int i = 0; i < 18; i++ ) {
        if (statusNames[i] == _name)
            return statusByte[i];
    }
    return 0;
}

// VIRTUAL PORT
MidiDevice::MidiDevice(void* _ctx, const std::string& _name) : 
    defaultOutChannel(0),
    defaultOutStatus(MidiDevice::CONTROLLER_CHANGE),
    tickCounter(0),
    midiIn(NULL), 
    midiOut(NULL) 
{
    type = DEVICE_MIDI;
    ctx = _ctx;
    name = _name;
}

// REAL PORT
MidiDevice::MidiDevice(void* _ctx, const std::string& _name, size_t _midiPort) : 
    defaultOutChannel(0),
    defaultOutStatus(MidiDevice::CONTROLLER_CHANGE),
    midiIn(NULL), 
    midiOut(NULL)
{
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

void MidiDevice::trigger(const unsigned char _status, unsigned char _channel) {
    // std::cout << " > " <<  name << " Status: " <<  statusByteToName(_status) << " Channel: " << (size_t)_channel << std::endl;

    std::vector<unsigned char> msg;
    msg.push_back( _status );
    if (_channel > 0 && _channel < 16 )
        msg[0] += _channel-1;
    midiOut->sendMessage( &msg );   
}

void MidiDevice::trigger(const unsigned char _status, unsigned char _channel, size_t _key, size_t _value) {
    // std::cout << " > " <<  name << " Status: " <<  statusByteToName(_status) << " Channel: " << (size_t)_channel << " Key: " << _key << " Value:" << _value << std::endl;
    
    std::vector<unsigned char> msg;
    msg.push_back( _status );

    if (_status != MidiDevice::TIMING_TICK) {
        msg.push_back( _key );
        msg.push_back( _value );
    }

    if (_channel > 0 && _channel < 16 )
        msg[0] += _channel-1;

    midiOut->sendMessage( &msg );   
}

void extractHeader(std::vector<unsigned char>* _message, unsigned char& _channel, unsigned char& _status, int& _bytes) {
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
        case MidiDevice::CONTROLLER_CHANGE:
            _bytes = 2;
            break;

        case MidiDevice::NOTE_ON:
            _bytes = 2;
            break;

        case MidiDevice::NOTE_OFF:
            _bytes = 2;
            break;

        case MidiDevice::KEY_PRESSURE:
            _bytes = 2;
            break;

        case MidiDevice::PROGRAM_CHANGE:
            _bytes = 1;
            break;

        case MidiDevice::CHANNEL_PRESSURE:
            _bytes = 2;
            break;

        case MidiDevice::PITCH_BEND:
            _bytes = 2;
            break;

        case MidiDevice::SONG_POSITION:
            _bytes = 2;
            break;

        case MidiDevice::SONG_SELECT:
            _bytes = 2;
            break;

        case MidiDevice::TUNE_REQUEST:
            _bytes = 2;
            break;

        case MidiDevice::TIMING_TICK:
            _bytes = 0;
            break;

        case MidiDevice::START_SONG:
            _bytes = 0;
            break;

        case MidiDevice::CONTINUE_SONG:
            _bytes = 0;
            break;

        case MidiDevice::STOP_SONG:
            _bytes = 0;
            break;

        case MidiDevice::SYSTEM_EXCLUSIVE:
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

    if (_status == MidiDevice::NOTE_ON && 
        _message->at(2) == 0) {
        _status = MidiDevice::NOTE_OFF;
    }
}

std::vector<std::string> MidiDevice::getInPorts() {
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

std::vector<std::string> MidiDevice::getOutPorts() {
    std::vector<std::string> devices;

    RtMidiOut* midiOut = new RtMidiOut();
    unsigned int nPorts = midiOut->getPortCount();

    for(unsigned int i = 0; i < nPorts; i++) {
        std::string name = midiOut->getPortName(i);
        stringReplace(name, '_');
        devices.push_back( name );
    }

    delete midiOut;

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

    int bytes = 0;
    unsigned char status = 0;
    unsigned char channel = 0;
    extractHeader(_message, channel, status, bytes);

    if (bytes < 2) {
        if (context->doStatusExist(device->name, status)) {
            YAML::Node node = context->getStatusNode(device->name, status);

            size_t target_value = 0;
            if (status == MidiDevice::TIMING_TICK) {
                target_value = device->tickCounter;
                device->tickCounter++;
                if (device->tickCounter > 127)
                    device->tickCounter = 0;
            }
            else if (status == MidiDevice::PROGRAM_CHANGE)
                target_value = _message->at(1);

            context->configMutex.lock();
            context->processEvent(node, device->name, status, 0, 0, target_value, true);
            context->configMutex.unlock();
        }
    }
    else {
        size_t key = _message->at(1);
        size_t target_value = _message->at(2);

        if (context->doKeyExist(device->name, (size_t)channel, key)) {
            YAML::Node node = context->getKeyNode(device->name, (size_t)channel, key);

            if (node["status"].IsDefined()) {
                unsigned char target_status = statusNameToByte(node["status"].as<std::string>());
                if (target_status != status)
                    return;
            }
            
            context->configMutex.lock();
            context->processEvent(node, device->name, status, (size_t)channel, key, (float)target_value, false);
            context->configMutex.unlock();
        }
    }
}


