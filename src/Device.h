#pragma once

#include "Term.h"

#include <map>
#include "ops/nodes.h"

class Device : public Term {
public:

    // KEYS EVENTS
    virtual void    setKeyFnc(size_t _channel, size_t _key, size_t _fnc) {
        size_t offset = _channel * 127;
        m_keyMap[offset + _key] = _fnc;
    }

    virtual bool    isKeyFnc(size_t _channel, size_t _key) const {
        // Channel 0 replay to all 
        if (m_keyMap.find(_key) != m_keyMap.end())
            return true;
        
        size_t offset = _channel * 127;
        return m_keyMap.find(offset + _key) != m_keyMap.end();
    }

    virtual size_t  getKeyFnc(size_t _channel, size_t _key) const {
        // Channel 0 have precedent over channel specific
        if (m_keyMap.find(_key) != m_keyMap.end())
            return m_keyMap.at(_key);

        size_t offset = size_t(_channel) * 127;
        return m_keyMap.at(offset + _key);
    }

    // STATUS ONLY EVENTS
    virtual void    setStatusFnc(unsigned char _status, size_t _fnc) { m_statusMap[_status] = _fnc; }
    virtual bool    isStatusFnc(unsigned char _status) const { return (m_statusMap.find(_status) != m_statusMap.end()); }
    virtual size_t  getStatusFnc(unsigned char _status) const { return m_statusMap.at(_status); }

    YAML::Node      node;

protected:
    std::map<size_t, size_t>            m_keyMap;
    std::map<unsigned char, size_t>     m_statusMap;
};
