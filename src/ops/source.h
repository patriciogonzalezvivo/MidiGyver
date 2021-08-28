#pragma once

#include <string>
#include "protocol.h"
#include "../Term.h"

struct Source {
    Protocol        protocol    = UNKNOWN_PROTOCOL;
    std::string     address     = "localhost";
    Term*           term        = nullptr;
    bool            isFile      = false;
};

inline Source parseSource(const std::string _address) {
    Source source;

    size_t post_protocol = 6;               // index position after protocol. Ex: 'abc://'
    std::string protocol = _address.substr(0,3);

    if (protocol == "mid") {
        source.protocol = MIDI_PROTOCOL;    // Protocol
        post_protocol = 7;                  // give space for 'midi://'
        // source.port = "0";                  // Channel
        // source.folder = "/cc";              // MessageType
    }
    else if (protocol == "csv") {
        source.protocol = CSV_PROTOCOL;
        source.address = "cout";
        if (_address.size() == 3)
            return source;
    }
    else if (protocol == "udp")
        source.protocol = UDP_PROTOCOL;
    else if (protocol == "osc")
        source.protocol = OSC_PROTOCOL;
    else
        return source;

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
        source.address = address.substr(0, addressEnd);

    if (source.address.find('.') == source.address.size() - 4 ||
        source.address.find('.') == source.address.size() - 5) {
        source.isFile = true;
    }

    else {

        // if (portStart != portEnd)
        //     source.port = address.substr(portStart, portEnd - portStart);

        // if (portEnd != total)
        //     source.folder = address.substr(portEnd, total - portEnd);

        if (source.address == "localhost")
            source.address = "127.0.0.1";

    }

    return source;
}