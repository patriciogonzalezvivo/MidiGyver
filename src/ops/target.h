#pragma once


#ifndef WIN32
#include <unistd.h>
#endif

#include <string>

enum TargetProtocol {
    UNKNOWN_PROTOCOL    = 0,
    CSV_PROTOCOL        = 1,
    UDP_PROTOCOL        = 2,
    OSC_PROTOCOL        = 3
};

struct Target {
    TargetProtocol protocol = UNKNOWN_PROTOCOL;
    std::string address = "localhost";
    std::string port    = "8000";
    std::string folder  = "/";
};

inline Target parseTarget(const std::string _address) {
    Target target;

    std::string protocol = _address.substr(0,3);

    if (protocol == "csv") {
        target.protocol = CSV_PROTOCOL;
        return target;
    }
    else if (protocol == "udp")
        target.protocol = UDP_PROTOCOL;
    else if (protocol == "osc")
        target.protocol = OSC_PROTOCOL;
    else
        return target;

    std::string address = _address.substr(6, _address.size() - 6);
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

    if (portStart != portEnd)
        target.port = address.substr(portStart, portEnd - portStart);

    if (portEnd != total)
        target.folder = address.substr(portEnd, total - portEnd);


    if (target.address == "localhost")
        target.address = "127.0.0.1";

    // std::cout << target.protocol << " " << target.address << " " << target.port  << " " << target.folder  << std::endl;

    return target;
}