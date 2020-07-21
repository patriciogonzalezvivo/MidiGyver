#pragma once

#include <string>
#include "../types/Color.h"
#include "../types/Vector.h"

#include "lo/lo.h"

inline void sendValue(const std::string& _address, const std::string& _port, const std::string& _path, float _value) {
    lo_message m = lo_message_new();
    lo_message_add_float(m, _value);

    lo_address t = lo_address_new(_address.c_str(), _port.c_str());

    lo_send_message(t, _path.c_str(), m);
    lo_address_free(t);
    lo_message_free(m);
}

inline void sendValue(const std::string& _address, const std::string& _port, const std::string& _path, const std::string& _value) {
    lo_message m = lo_message_new();
    lo_message_add_string(m, _value.c_str());

    lo_address t = lo_address_new(_address.c_str(), _port.c_str());

    lo_send_message(t, _path.c_str(), m);
    lo_address_free(t);
    lo_message_free(m);
}

inline void sendValue(const std::string& _address, const std::string& _port, const std::string& _path, Vector _value) {
    lo_message m = lo_message_new();
    lo_message_add_float(m, _value.x);
    lo_message_add_float(m, _value.y);
    lo_message_add_float(m, _value.z);

    lo_address t = lo_address_new(_address.c_str(), _port.c_str());

    lo_send_message(t, _path.c_str(), m);
    lo_address_free(t);
    lo_message_free(m);
}

inline void sendValue(const std::string& _address, const std::string& _port, const std::string& _path, Color _value) {
    lo_message m = lo_message_new();
    lo_message_add_float(m, _value.r);
    lo_message_add_float(m, _value.g);
    lo_message_add_float(m, _value.b);
    lo_message_add_float(m, _value.a);

    lo_address t = lo_address_new(_address.c_str(), _port.c_str());

    lo_send_message(t, _path.c_str(), m);
    lo_address_free(t);
    lo_message_free(m);
}
