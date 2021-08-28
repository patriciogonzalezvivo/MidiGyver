#include <sys/stat.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <iostream>
#include <fstream>

#include "Context.h"
#include "Command.h"
#include "ops/strings.h"

CommandList commands;
Context*    ctx; 
std::string version = "0.9";
std::string name = "midigyver";
std::string header = name + " " + version + " by Patricio Gonzalez Vivo ( patriciogonzalezvivo.com )"; 
std::string configfile = "";
std::atomic<bool> bRun(true);
std::mutex  contextMutex;

// CONSOLE IN watcher
void cinWatcherThread();

int main(int argc, char** argv) {
    if (argc == 1) {
        std::cout << "Use: " << std::string(argv[0]) << " config.yaml " << std::endl;
        return 0;
    }
    
    configfile = std::string(argv[1]);

    commands.push_back(Command("help", [&](const std::string& _line){
        if (_line == "help") {
            std::cout << "// " << header << std::endl;
            std::cout << "// " << std::endl;
            for (unsigned int i = 0; i < commands.size(); i++) {
                std::cout << "// " << commands[i].description << std::endl;
            }
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',', true);
            if (values.size() == 2) {
                for (unsigned int i = 0; i < commands.size(); i++) {
                    if (commands[i].begins_with == values[1]) {
                        std::cout << "// " << commands[i].description << std::endl;
                    }
                }
            }
        }
        return false;
    },
    "help[,<command>]               print help for one or all command"));

    commands.push_back(Command("source", [&](const std::string& _line){ 
        if (_line == "sources") {
            std::cout << std::endl << "Sources:" << std::endl;
            std::cout << "----------------------------" << std::endl;
            for (size_t i = 0; i < ctx->sources.size(); i++)
                std::cout << ctx->sources[i].address << std::endl;
            std::cout << std::endl;
            
            return true;
        }
        return false;
    },
    "version                        return version."));

    commands.push_back(Command("target", [&](const std::string& _line){ 
        if (_line == "targets") {
            std::cout << std::endl << "Targets:" << std::endl;
            std::cout << "----------------------------" << std::endl;
            for (size_t i = 0; i < ctx->targets.size(); i++)
                std::cout << ctx->targets[i].address << std::endl;
            std::cout << std::endl;
            
            return true;
        }
        return false;
    },
    "version                        return version."));

    commands.push_back(Command("version", [&](const std::string& _line){ 
        if (_line == "version") {
            std::cout << version << std::endl;
            return true;
        }
        return false;
    },
    "version                        return version."));

    commands.push_back(Command("q", [&](const std::string& _line){ 
        if (_line == "q") {
            bRun.store(false);
            return true;
        }
        return false;
    },
    "q                              close"));

    commands.push_back(Command("quit", [&](const std::string& _line){ 
        bRun.store(false);
        return true;
    },
    "quit                           close"));

    commands.push_back(Command("exit", [&](const std::string& _line){ 
        bRun.store(false);
        return true;
    },
    "exit                           close"));

    commands.push_back(Command("save", [&](const std::string& _line){
        if (_line == "save") {
            ctx->save(configfile);
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',',true);
            if (values.size() == 2) {
                ctx->save(values[1]);
            }
        }
        return false;
    },
    "save                           save values"));

    struct stat st;
    int lastChange;
    bool fileChanged = false;

    ctx = new Context();    
    if (ctx->load(configfile)) {
        stat( configfile.c_str(), &st );
        lastChange = st.st_mtime;
    }

    std::thread cinWatcher( &cinWatcherThread );

    // Watch for MIDI deivces or file changes
    size_t lastMidiIn = MidiDevice::getInPorts().size();
    while (bRun) {

        stat( configfile.c_str(), &st );
        int date = st.st_mtime;
        size_t midiIn = MidiDevice::getInPorts().size();
        if ( date != lastChange || lastMidiIn != midiIn) {
            contextMutex.lock();
            lastChange = date;
            lastMidiIn = midiIn;
            ctx->close();
            ctx->load(configfile);
            contextMutex.unlock();
        }

        #if defined(_WIN32)
        std::this_thread::sleep_for(std::chrono::microseconds(1000000));
        #else
        usleep(1000000);
        #endif 

    }

    ctx->close();

#ifndef _WIN32
    pthread_t cinHandler = cinWatcher.native_handle();
    pthread_cancel( cinHandler );
#endif

    exit(0);
}

void cinWatcherThread() {// Commands comming from the console IN
    std::cout << "// > ";
    std::string console_line;
    while (std::getline(std::cin, console_line)) {

        for (size_t i = 0; i < commands.size(); i++) {
            if (beginsWith(console_line, commands[i].begins_with)) {
                // If got resolved stop 
                contextMutex.lock();
                bool resolve = commands[i].exec(console_line);
                contextMutex.unlock();
                if (resolve)
                    break;
            }
        }

        std::cout << "// > ";
    }
}
