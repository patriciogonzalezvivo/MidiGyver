#include "MidiDevice.h"

#include "ops/values.h"
#include "ops/strings.h"

#include "types/Color.h"
#include "types/Vector.h"

#include "Context.h"

#include <thread>
#include <chrono>

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

// VIRTUAL PORT
MidiDevice::MidiDevice(void* _ctx, const std::string& _name) : 
    midiPort(0),
    defaultOutChannel(0),
    defaultOutStatus(Midi::CONTROLLER_CHANGE),
    tickCounter(0),
    midiIn(NULL), 
    midiOut(NULL) 
{
    type = MIDI_DEVICE;
    ctx = _ctx;
    name = _name;
}

// REAL PORT
MidiDevice::MidiDevice(void* _ctx, const std::string& _name, size_t _midiPort) : 
    defaultOutChannel(0),
    defaultOutStatus(Midi::CONTROLLER_CHANGE),
    midiIn(NULL), 
    midiOut(NULL)
{
    type = MIDI_DEVICE;
    ctx = _ctx;
    name = _name;
    midiPort = _midiPort;

    openInPort(_name, _midiPort);
    openOutPort(_name, _midiPort);
}

MidiDevice::~MidiDevice() {
    close();
}

bool MidiDevice::openInPort(const std::string& _name, size_t _midiPort) {
    try {
        midiIn = new RtMidiIn(RtMidi::Api(0), "midigyver");
    } catch(RtMidiError &error) {
        error.printMessage();
        return false;
    }

    midiIn->openPort(_midiPort, _name);
    midiIn->ignoreTypes(false, false, true);
    midiIn->setCallback(onMidi, this);
    // midiIn->setCallback( , this);

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

bool MidiDevice::close() {
    if (midiIn) {
        midiIn->cancelCallback();
        midiIn->closePort();
        delete midiIn;
        midiIn = NULL;
    }

    if (midiOut) {
        midiOut->closePort();
        delete midiOut;
        midiOut = NULL;
    }

    midiPort = 0;
    tickCounter = 0;
    defaultOutChannel = 0;
    defaultOutStatus = Midi::CONTROLLER_CHANGE;

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
    
    unsigned char s = _status;
    if (s == Midi::NOTE_ON && _value == 0)
        s = Midi::NOTE_OFF;

    std::vector<unsigned char> msg;
    msg.push_back( s );

    if (s != Midi::TIMING_TICK) {
        msg.push_back( _key );
        msg.push_back( _value );
    }

    if (_channel > 0 && _channel < 16 )
        msg[0] += _channel-1;

    midiOut->sendMessage( &msg );   
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
    Midi::extractHeader(_message, channel, status, bytes);

    if (bytes < 2) {
        if (context->doStatusExist(device->name, status)) {
            YAML::Node node = context->getStatusNode(device->name, status);

            size_t target_value = 0;
            if (status == Midi::TIMING_TICK) {
                target_value = device->tickCounter;
                device->tickCounter++;
                if (device->tickCounter > 127)
                    device->tickCounter = 0;
            }
            else if (status == Midi::PROGRAM_CHANGE)
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


