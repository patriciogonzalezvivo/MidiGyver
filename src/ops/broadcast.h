#pragma once

#include "target.h"

#include "udp.h"
#include "osc.h"

template <typename T>
inline void broadcast(const std::string& _address, const std::string& _prop, const T& _value) {
    Target target = parseTarget(_address);

    if (target.protocol == UNKNOWN_PROTOCOL) {
        std::cout << "UNKNOWN PROTOCOL for " << _prop << " " << _value << std::endl;
    }
    else if (target.protocol == CSV_PROTOCOL) {
        std::cout << _prop << "," << _value << std::endl;
    }
    else if (target.protocol == OSC_PROTOCOL) {
        broadcast_OSC(target, _prop, _value);
    }
    else if (target.protocol == UDP_PROTOCOL) {
        broadcast_UDP(target, _prop, _value);
    }

    return;
}