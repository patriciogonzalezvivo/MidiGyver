#pragma once

#include "target.h"

#include "udp.h"
#include "osc.h"

#include <iostream>
#include <fstream>

template <typename T>
inline bool broadcast(const Target& _target, const std::string& _prop, const T& _value) {
    if (_target.protocol == UNKNOWN_PROTOCOL) {
        std::cout << "UNKNOWN PROTOCOL for " << _prop << " " << _value << std::endl;
        return true;
    }
    else if (_target.protocol == CSV_PROTOCOL) {
        if (_target.isFile) {
            std::ofstream file;
            file.open (_target.address, std::ios_base::app);
            file << _prop << "," << _value << std::endl;
            file.close();
        }
        else {
            std::cout << _prop << "," << _value << std::endl;
        }

        return true;
    }
    else if (_target.protocol == OSC_PROTOCOL) {
        return broadcast_OSC(_target, _prop, _value);
    }
    else if (_target.protocol == UDP_PROTOCOL) {
        return broadcast_UDP(_target, _prop, _value);
    }
    return false;
}

template <typename T>
inline bool broadcast(const std::string& _address, const std::string& _prop, const T& _value) {
    Target target = parseTarget(_address);
    return broadcast(target, _prop, _value);
}