#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <vector>

#include "RtMidi.h"
#include "yaml-cpp/yaml.h"

#include "MidiDevice.h"
#include "tools.h"

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
    std::string configfile = "";

    if (argc == 1) {
        std::cout << "Use: " << std::string(argv[0]) << " config.yaml " << std::endl;
        return 0;
    }
    
    configfile = std::string(argv[1]);

    YAML::Node node = YAML::LoadFile(configfile);
    std::vector<std::string> devices = getInputPorts();

    std::vector<MidiDevice*> inputs;
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

    for (size_t i = 0; i < inputs.size(); i++)
        node[ inputs[i]->broadcaster.deviceName ] = inputs[i]->broadcaster.data;

    YAML::Emitter out;
    out.SetIndent(4);
    out.SetSeqFormat(YAML::Flow);
    out << node;

    std::ofstream fout(configfile);
    fout << out.c_str();

    for (std::vector<MidiDevice*>::iterator inputIterator = inputs.begin(); inputIterator < inputs.end(); inputIterator++)
        delete *inputIterator;
}
