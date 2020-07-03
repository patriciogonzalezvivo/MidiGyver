#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <vector>

#include "RtMidi.h"
#include "MidiInput.h"

void tokenize(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters = " ") {
    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos) {
        // Found a token, add it to the std::vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

std::vector<int> selectPorts() {
    int port;
    std::vector<std::string>::iterator it;
    std::string userInput;
    std::vector<std::string>userInputTokenized;
    std::vector<int> portList;

    std::cout << "Select ports: ";
    getline(std::cin, userInput);
    tokenize(userInput, userInputTokenized);

    for (it = userInputTokenized.begin(); it < userInputTokenized.end(); it++) {
        std::stringstream(*it) >> port;
        portList.push_back(port);
    }
    return portList;
}

void listInputPorts() {
    RtMidiIn* midiIn = new RtMidiIn();
    unsigned int nPorts = midiIn->getPortCount();
    std::cout << "Found " << nPorts << " MIDI inputs." << std::endl;
    std::string portName;
    if(nPorts == 0) {
        std::cout << "No ports available!\n";
    } else {
        for(unsigned int i = 0; i < nPorts; i++) {
            portName = midiIn->getPortName(i);
            std::cout << "   Input port #" << i+1 << ": " << portName << std::endl;
        }
    }
    delete midiIn;
}

std::map<std::string, int> populateMap() {
    std::map<std::string, int> stringToStatus;
    stringToStatus["note_off"] = 0x80;
    stringToStatus["note_on"] = 0x90;
    stringToStatus["key_pressure"] = 0xa0;
    stringToStatus["controller_change"] = 0xb0;
    stringToStatus["program_change"] = 0xc0;
    stringToStatus["channel_pressure"] = 0xd0;
    stringToStatus["pitch_bend"] = 0xe0;
    stringToStatus["system_exclusive"] = 0xf0;
    stringToStatus["song_position"] = 0xf2;
    stringToStatus["song_select"] = 0xf3;
    stringToStatus["tune_request"] = 0xf6;
    stringToStatus["end_of_sysex"] = 0xf7;
    stringToStatus["timing_tick"] = 0xf8;
    stringToStatus["start_song"] = 0xfa;
    stringToStatus["continue_song"] = 0xfb;
    stringToStatus["stop_song"] = 0xfc;
    return stringToStatus;
}

void stringReplace(std::string* str) {
    replace_if(str->begin(), str->end(), std::bind2nd(std::equal_to<char>(), '_'), ' ');
}

int main(int argc, char** argv) {
    std::vector<MidiInput*> inputs;
    std::vector<int> portList;
    std::vector<int>::iterator portIterator;
    std::vector<MidiInput*>::iterator inputIterator;
    std::string configfile = "";

    for (int i = 1; i < argc; i++) {
        configfile = std::string(argv[i]);
    }

    listInputPorts();
    portList = selectPorts();

    for (portIterator = portList.begin(); portIterator < portList.end(); portIterator++) {
        *portIterator -= 1;
        MidiInput* m  = new MidiInput(configfile, *portIterator);
        inputs.push_back(m);
    }

    std::cout << std::endl << "Reading MIDI input ... press <enter> to quit." << std::endl;
    char input;
    std::cin.get(input);

    for (inputIterator = inputs.begin(); inputIterator < inputs.end(); inputIterator++) {
        delete *inputIterator;
    }
}
