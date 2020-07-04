#include "MidiInput.h"

MidiInput::MidiInput(const std::string& _filename, int _MidiPort) {
    try {
        midiIn = new RtMidiIn();
    } catch(RtMidiError &error) {
        error.printMessage();
    }

    midiIn->openPort(_MidiPort);

    std::string portName;
    portName = midiIn->getPortName(_MidiPort);
    stringReplace(&portName, '_');

    broadcaster.load(_filename, portName);

    midiIn->setCallback(onMidi, &broadcaster);

    //respond to sysex and timing, ignore active sensing
    midiIn->ignoreTypes(false, false, true);
    std::cout << "Listening to port " << midiIn->getPortName(_MidiPort) << std::endl;
}

MidiInput::~MidiInput() {
    delete this->midiIn;
}

void MidiInput::stringReplace(std::string* str, char rep) {
    replace_if(str->begin(), str->end(), std::bind2nd(std::equal_to<char>(), ' '), rep);
    replace_if(str->begin(), str->end(), std::bind2nd(std::equal_to<char>(), ':'), rep);
}

void MidiInput::onMidi(double _deltatime, std::vector<unsigned char>* _message, void* _userData) {
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
