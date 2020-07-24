#pragma once

#include <string>
#include "../types/Color.h"
#include "../types/Vector.h"

#include "lo/lo.h"

struct OscTarget {
    std::string address = "localhost";
    std::string port    = "8000";
    std::string folder  = "/";
};

inline OscTarget parseOscTarget(const std::string _address) {
    OscTarget target;

    std::string address = _address.substr(6, _address.size() - 6);
    std::size_t addressEnd = address.find(":");
    std::size_t portStart = addressEnd+1;
    std::size_t portEnd = address.find("/"); 
    size_t total = address.size();

    if (portEnd == std::string::npos) {
        portStart = total;
        portEnd = total;
    }

    if (addressEnd == std::string::npos)
        addressEnd = portStart;

    if (addressEnd != 0)
        target.address = address.substr(0, addressEnd);

    if (portEnd != total)
        target.port = address.substr(portStart, portEnd - portStart);

    if (portEnd != total)
        target.folder = address.substr(portEnd, total - portEnd);

    // std::cout << target.address << " " << target.port  << " " << target.folder  << std::endl;

    return target;
}

inline void sendValue(const OscTarget& _osc, const std::string& _folder, float _value) {
    lo_message m = lo_message_new();
    lo_message_add_float(m, _value);

    lo_address t = lo_address_new(_osc.address.c_str(), _osc.port.c_str());

    lo_send_message(t, std::string(_osc.folder + _folder).c_str(), m);
    lo_address_free(t);
    lo_message_free(m);
}

inline void sendValue(const OscTarget& _osc, const std::string& _folder, const std::string& _value) {
    lo_message m = lo_message_new();
    lo_message_add_string(m, _value.c_str());

    lo_address t = lo_address_new(_osc.address.c_str(), _osc.port.c_str());

    lo_send_message(t, std::string(_osc.folder + _folder).c_str(), m);
    lo_address_free(t);
    lo_message_free(m);
}

inline void sendValue(const OscTarget& _osc, const std::string& _folder, Vector _value) {
    lo_message m = lo_message_new();
    lo_message_add_float(m, _value.x);
    lo_message_add_float(m, _value.y);
    lo_message_add_float(m, _value.z);

    lo_address t = lo_address_new(_osc.address.c_str(), _osc.port.c_str());

    lo_send_message(t, std::string(_osc.folder + _folder).c_str(), m);
    lo_address_free(t);
    lo_message_free(m);
}

inline void sendValue(const OscTarget& _osc, const std::string& _folder, Color _value) {
    lo_message m = lo_message_new();
    lo_message_add_float(m, _value.r);
    lo_message_add_float(m, _value.g);
    lo_message_add_float(m, _value.b);
    lo_message_add_float(m, _value.a);

    lo_address t = lo_address_new(_osc.address.c_str(), _osc.port.c_str());

    lo_send_message(t, std::string(_osc.folder + _folder).c_str(), m);
    lo_address_free(t);
    lo_message_free(m);
}
