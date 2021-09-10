#pragma once

#include "Device.h"
#include <lo/lo_cpp.h>

class OscDevice : public Device {
public:

    OscDevice(void* _ctx, const std::string& _name, size_t _port);
    virtual ~OscDevice();
    virtual bool        close();

protected:
    lo::ServerThread*   m_in;
    int                 m_port;

};


