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
    m_in(NULL), 
    m_out(NULL),
    m_tickCounter(0)
{
    m_type = MIDI_DEVICE;
    m_name = _name;
    m_ctx = _ctx;
    m_port = 0;
}

// REAL PORT
MidiDevice::MidiDevice(void* _ctx, const std::string& _name, size_t _port) : 
    m_in(NULL), 
    m_out(NULL),
    m_tickCounter(0)
{
    m_type = MIDI_DEVICE;
    m_name = _name;
    m_ctx = _ctx;
    m_port = _port;

    openInPort(_name, _port);
    openOutPort(_name, _port);
}

MidiDevice::~MidiDevice() {
    close();
}

bool MidiDevice::openInPort(const std::string& _name, size_t _port) {
    try {
        m_in = new RtMidiIn(RtMidi::Api(0), "midigyver");
    } catch(RtMidiError &error) {
        error.printMessage();
        return false;
    }

    m_in->openPort(_port, _name);
    m_in->ignoreTypes(false, false, true);
    m_in->setCallback(onEvent, this);
    // m_in->setCallback( , this);

    // midiName = m_in->getPortName(_port);
    // stringReplace(midiName, '_');
    return true;
}

bool MidiDevice::openOutPort(const std::string& _name, size_t _port) {
    try {
        m_out = new RtMidiOut(RtMidi::Api(0), "midigyver");
        m_out->openPort(_port, _name);
    }
    catch(RtMidiError &error) {
        error.printMessage();
        return false;
    }

    return true;
}

bool MidiDevice::openVirtualOutPort(const std::string& _name) {
    try {
        m_out = new RtMidiOut(RtMidi::Api(0), "midigyver" );
        m_out->openVirtualPort(_name);
    }
    catch(RtMidiError &error) {
        error.printMessage();
        return false;
    }

    return true;
}

bool MidiDevice::close() {
    if (m_in) {
        m_in->cancelCallback();
        m_in->closePort();
        delete m_in;
        m_in = NULL;
    }

    if (m_out) {
        m_out->closePort();
        delete m_out;
        m_out = NULL;
    }

    m_port = 0;
    m_tickCounter = 0;

    return true;
}

void MidiDevice::trigger(const unsigned char _status, unsigned char _channel) {
    // std::cout << " > " <<  name << " Status: " <<  statusByteToName(_status) << " Channel: " << (size_t)_channel << std::endl;

    std::vector<unsigned char> msg;
    msg.push_back( _status );
    if (_channel > 0 && _channel < 16 )
        msg[0] += _channel-1;
    m_out->sendMessage( &msg );   
}

void MidiDevice::trigger(const unsigned char _status, unsigned char _channel, size_t _key, size_t _value) {
    // std::cout << " > " << m_name << " Status: " <<  statusByteToName(_status) << " Channel: " << (size_t)_channel << " Key: " << _key << " Value:" << _value << std::endl;
    
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

    m_out->sendMessage( &msg );   
}


void MidiDevice::onEvent(double _deltatime, std::vector<unsigned char>* _message, void* _userData) {
    unsigned int nBytes = 0;
    try {
        nBytes = _message->size();
    } 
    catch(RtMidiError &error) {
        error.printMessage();
        exit(EXIT_FAILURE);
    }

    MidiDevice *device = static_cast<MidiDevice*>(_userData);
    Context *context = static_cast<Context*>(device->m_ctx);

    int bytes = 0;
    unsigned char status = 0;
    unsigned char channel = 0;
    Midi::extractHeader(_message, channel, status, bytes);

    if (bytes < 2) {
        if (context->doStatusExist(device->getName(), status)) {
            YAML::Node node = context->getStatusNode(device->getName(), status);

            size_t target_value = 0;
            if (status == Midi::TIMING_TICK) {
                target_value = device->m_tickCounter;
                device->m_tickCounter++;
                if (device->m_tickCounter > 127)
                    device->m_tickCounter = 0;
            }
            else if (status == Midi::PROGRAM_CHANGE)
                target_value = _message->at(1);

            context->configMutex.lock();
            context->processEvent(node, device->getName(), status, 0, 0, target_value, true);
            context->configMutex.unlock();
        }
    }
    else {
        size_t key = _message->at(1);
        size_t target_value = _message->at(2);

        if (context->doKeyExist(device->getName(), (size_t)channel, key)) {
            YAML::Node node = context->getKeyNode(device->getName(), (size_t)channel, key);

            if (node["status"].IsDefined()) {
                unsigned char target_status = statusNameToByte(node["status"].as<std::string>());
                if (target_status != status)
                    return;
            }
            
            context->configMutex.lock();
            context->processEvent(node, device->getName(), status, (size_t)channel, key, (float)target_value, false);
            context->configMutex.unlock();
        }
    }
}


