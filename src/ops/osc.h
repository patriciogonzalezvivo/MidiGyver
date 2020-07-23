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
