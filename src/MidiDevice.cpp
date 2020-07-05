#include "MidiDevice.h"
#include "tools.h"

MidiDevice::MidiDevice(const std::string& _filename, int _MidiPort) {
    try {
        midiIn = new RtMidiIn();
    } catch(RtMidiError &error) {
        error.printMessage();
    }
    midiIn->openPort(_MidiPort);

    try {
        midiOut = new RtMidiOut();
        midiOut->openPort(_MidiPort);
    } catch(RtMidiError &error) {
        error.printMessage();
        midiOut = nullptr;
    }

    std::string portName = midiIn->getPortName(_MidiPort);
    stringReplace(portName, '_');

    broadcaster.setCallbackPort(midiOut);
    broadcaster.load(_filename, portName);

    midiIn->setCallback(onMidi, &broadcaster);
    midiIn->ignoreTypes(false, false, true);
}

MidiDevice::~MidiDevice() {
    delete this->midiIn;
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

    Broadcaster *b = static_cast<Broadcaster*>(_userData);
    b->broadcast(_message);
}
