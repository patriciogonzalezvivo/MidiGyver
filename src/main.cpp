#include "Context.h"

#include "MidiDevice.h"
#include "rtmidi/RtMidi.h"
#include "ops/nodes.h"

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
    if (argc == 1) {
        std::cout << "Use: " << std::string(argv[0]) << " config.yaml " << std::endl;
        return 0;
    }
    
    std::string configfile = std::string(argv[1]);

    Context ctx;    
    if (ctx.load(configfile)) {

        std::vector<MidiDevice*> inputs;
        std::vector<std::string> devices_in = getInputPorts();
        for (size_t i = 0; i < devices_in.size(); i++) {
            stringReplace(devices_in[i], '_');
            std::string deviceKeyName = getMatchingKey(ctx.config["in"], devices_in[i]);

            if (deviceKeyName.size() > 0) {
                ctx.devices.push_back(deviceKeyName);
                ctx.updateDevice(deviceKeyName);
                inputs.push_back(new MidiDevice(&ctx, deviceKeyName, i));
            }
        }

        if (inputs.size() == 0) {
            std::cout << "Listening to no device. Please load the config.yaml for: " << std::endl;
            for (size_t i = 0; i < devices_in.size(); i++)
                std::cout << "  - " << devices_in[i] << std::endl;

            return false;
        }

        char input;
        std::cin.get(input);
        ctx.save(configfile);

        for (std::vector<MidiDevice*>::iterator inputIterator = inputs.begin(); inputIterator < inputs.end(); inputIterator++)
            delete *inputIterator;
    }

    return 1;
}
