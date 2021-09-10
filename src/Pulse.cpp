#include "Pulse.h"

#include <chrono>
#include <string>

#include "Context.h"

Pulse::Pulse(void* _ctx, size_t _index) : m_clear (false) {
    m_index = _index;
    m_ctx   = _ctx;
    m_type = PULSE;
    m_name = ((Context*)m_ctx)->config["pulse"][m_index]["name"].as<std::string>();
}   

Pulse::~Pulse() {
    close();
}

bool Pulse::close() {
    stop();
    return true;
}

void Pulse::start(size_t _milliSec) {
    this->m_clear = false;

    m_thread = std::thread([=]() {
        float counter = 0.0;
        while (true && !this->m_clear) {
            if (this->m_clear) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(_milliSec));
            if (this->m_clear) break;
            
            if (((Context*)m_ctx)->safe) {
                ((Context*)m_ctx)->configMutex.lock();
                ((Context*)m_ctx)->processEvent(((Context*)m_ctx)->config["pulse"][m_index], m_name, Midi::TIMING_TICK, 0, 0, counter, true);
                ((Context*)m_ctx)->configMutex.unlock();
            }

            counter++;
            if (counter > 127.0)
                counter = 0.0;
        }
    });
    // t.detach();
}

void Pulse::stop() {
    m_clear = true;
    m_thread.join();
}