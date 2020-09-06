#include "Pulse.h"

#include <thread>
#include <chrono>
#include <string>

#include "Context.h"

Pulse::Pulse(void* _ctx, size_t _index) {
    type = DEVICE_PULSE;
    ctx = _ctx;
    name = ((Context*)ctx)->config["pulse"][_index]["name"].as<std::string>();
    keyMap[_index] = _index;
    index = _index;
}   

Pulse::~Pulse() {
}

void Pulse::start(size_t _milliSec) {
    this->clear = false;

    std::thread t([=]() {
        float counter = 0.0;
        while(true) {
            if(this->clear) return;
            std::this_thread::sleep_for(std::chrono::milliseconds(_milliSec));
            if(this->clear) return;
            
            ((Context*)ctx)->configMutex.lock();
            if (((Context*)ctx)->shapeKeyValue(((Context*)ctx)->config["pulse"][index], name, "pulse", 0, &counter))
                        ((Context*)ctx)->mapKeyValue(((Context*)ctx)->config["pulse"][index], name, 0, counter);
            ((Context*)ctx)->configMutex.unlock();

            counter++;
            if (counter > 127.0)
                counter = 0.0;
        }
    });
    t.detach();
}

void Pulse::stop() {
    clear = true;
}