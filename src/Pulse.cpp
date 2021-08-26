#include "Pulse.h"

#include <chrono>
#include <string>

#include "Context.h"

Pulse::Pulse(void* _ctx, size_t _index) {
    type = DEVICE_PULSE;
    ctx = _ctx;
    defaultOutChannel = 0;
    name = ((Context*)ctx)->config["pulse"][_index]["name"].as<std::string>();
    // setKeyFnc(0, _index, _index);
    setStatusFnc(Midi::TIMING_TICK, _index);
    index = _index;
}   

Pulse::~Pulse() {
}

void Pulse::start(size_t _milliSec) {
    this->clear = false;

    t = std::thread([=]() {
        float counter = 0.0;
        while (true && !this->clear) {
            if (this->clear) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(_milliSec));
            if (this->clear) break;
            
            if (((Context*)ctx)->safe) {
                ((Context*)ctx)->configMutex.lock();
                ((Context*)ctx)->processEvent(((Context*)ctx)->config["pulse"][index], name, Midi::TIMING_TICK, 0, 0, counter, true);
                ((Context*)ctx)->configMutex.unlock();
            }

            counter++;
            if (counter > 127.0)
                counter = 0.0;
        }
    });
    // t.detach();
}

void Pulse::stop() {
    clear = true;
    t.join();
}