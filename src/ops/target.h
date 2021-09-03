#pragma once

#ifndef WIN32
#include <unistd.h>
#endif

#include <string>
#include "protocol.h"
#include "source.h"
#include "../term.h"

struct Target {
    Target() {};
    Target(const Source &_p) {
        protocol = _p.protocol;
        address = _p.address;
        term = _p.term;
        isFile = _p.isFile;

        if (protocol == MIDI_PROTOCOL && isFile) 
            folder = "CONTROLLER_CHANGE"; 
    };

    Protocol        protocol    = UNKNOWN_PROTOCOL;
    std::string     address     = "localhost";
    std::string     port        = "8000";
    std::string     folder      = "/";
    Term*           term        = nullptr;
    bool            isFile      = false;
};

inline Target parseTarget(const std::string _address) {
    Target target;

    size_t post_protocol = 6;               // index position after protocol. Ex: 'abc://'
    std::string protocol = _address.substr(0,3);

    if (protocol == "mid") {
        target.protocol = MIDI_PROTOCOL;    // Protocol
        post_protocol = 7;                  // give space for 'midi://'
        target.port = "0";                  // Channel
        // target.folder = "/";                // MessageType
    }
    else if (protocol == "csv") {
        target.protocol = CSV_PROTOCOL;
        target.address = "cout";
        if (_address.size() == 3)
            return target;
    }
    else if (protocol == "udp")
        target.protocol = UDP_PROTOCOL;
    else if (protocol == "osc")
        target.protocol = OSC_PROTOCOL;
    else
        return target;

    std::string address = _address.substr(post_protocol, _address.size() - post_protocol);
    std::size_t addressEnd = address.find(":");
    std::size_t portStart = addressEnd+1;
    std::size_t portEnd = address.find("/"); 
    size_t total = address.size();

    if (addressEnd == std::string::npos) {
        addressEnd = total;
        portStart = total;
        portEnd = total;
    }
    else if (portEnd == std::string::npos) {
        portEnd = total;
    }

    if (addressEnd != 0)
        target.address = address.substr(0, addressEnd);

    if (target.address.find('.') == target.address.size() - 4 ||
        target.address.find('.') == target.address.size() - 5) {
        target.isFile = true;
    }

    else {

        if (portStart != portEnd)
            target.port = address.substr(portStart, portEnd - portStart);

        if (portEnd != total)
            target.folder = address.substr(portEnd, total - portEnd);

    }

    if (target.protocol == MIDI_PROTOCOL)
        target.folder.erase(0,1);

    // std::cout << (target.isFile ? "FILE " : "NOT FILE  ") << target.protocol << " " << target.address << " " << target.port  << " " << target.folder  << std::endl;

    return target;
}