#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <vector>

#include "RtMidi.h"
#include "MidiDevice.h"

#include "tools.h"

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

std::vector<std::string> getInputPorts() {
    std::vector<std::string> devices;

    RtMidiIn* midiIn = new RtMidiIn();
    unsigned int nPorts = midiIn->getPortCount();

    for(unsigned int i = 0; i < nPorts; i++)
        devices.push_back( midiIn->getPortName(i) );

    delete midiIn;

    return devices;
}

int main(int argc, char** argv) {
    std::vector<MidiDevice*> inputs;
    std::vector<int> portList;
    std::vector<int>::iterator portIterator;
    std::string configfile = "";

    if (argc == 1) {
        std::cout << "Use: " << std::string(argv[0]) << " config.yaml " << std::endl;
        return 0;
    }
    
    configfile = std::string(argv[1]);

    YAML::Node node = YAML::LoadFile(configfile);
    std::vector<std::string> devices = getInputPorts();

    for (size_t i = 0; i < devices.size(); i++) {
        std::string device = devices[i];
        stringReplace( device, '_');
        if (node[device]) {
            MidiDevice* m  = new MidiDevice(configfile, i);
            inputs.push_back(m);
        }
    }

    if (inputs.size() == 0) {
        std::cout << "Listening to no device. Please load the config.yaml for: " << std::endl;
        for (size_t i = 0; i < devices.size(); i++)
            std::cout << "  - " << devices[i] << std::endl;
    }
    else {
        // std::cout << std::endl << "Reading MIDI input ... press <enter> to quit." << std::endl;
        char input;
        std::cin.get(input);
    }

    for (std::vector<MidiDevice*>::iterator inputIterator = inputs.begin(); inputIterator < inputs.end(); inputIterator++)
        delete *inputIterator;
}
