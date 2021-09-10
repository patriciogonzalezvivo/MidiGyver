#pragma once

#include <thread>
#include <functional>

#include "yaml-cpp/yaml.h"

#include "Term.h"

class Pulse : public Term {
public:

    Pulse(void* _ctx, size_t _index);
    virtual ~Pulse();
    virtual bool    close();

    void        start(size_t _milliSec);
    void        stop();

private:
    std::thread m_thread;
    size_t      m_index;
    bool        m_clear = false;
};