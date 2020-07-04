// Copyright (C) 2010 Jonny Stutters
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

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
