#pragma once

#include <map>

enum DeviceType {
    DEVICE_PULSE,
    DEVICE_MIDI
};

class Device {
public:

    std::string                 name;
    DeviceType                  type;

    // KEYS EVENTS
    void                        setKeyFnc(size_t _channel, size_t _key, size_t _fnc) {
        size_t offset = _channel * 127;
        keyMap[offset + _key] = _fnc;
    }

    bool                        isKeyFnc(size_t _channel, size_t _key) {
        // Channel 0 replay to all 
        //
        if (keyMap.find(_key) != keyMap.end())
            return true;
        
        size_t offset = _channel * 127;
        return keyMap.find(offset + _key) != keyMap.end();
    }

    size_t                      getKeyFnc(size_t _channel, size_t _key) {
        // Channel 0 have precedent over channel specific
        //
        if (keyMap.find(_key) != keyMap.end())
            return keyMap[_key];

        size_t offset = size_t(_channel) * 127;
        return keyMap[offset + _key];
    }


    // STATUS ONLY EVENTS
    void                        setStatusFnc(unsigned char _status, size_t _fnc) {
        statusMap[_status] = _fnc;
    }

    bool                        isStatusFnc(unsigned char _status) {
        if (statusMap.find(_status) == statusMap.end())
            return false;
        return true;
    }

    size_t                      getStatusFnc(unsigned char _status) {
        return statusMap[_status];
    }

protected:
    std::map<size_t, size_t>            keyMap;
    std::map<unsigned char, size_t>     statusMap;
    void*                               ctx;
};
