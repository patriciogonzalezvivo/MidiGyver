#include "Context.h"

#include "ops/broadcast.h"

#include "ops/nodes.h"
#include "ops/strings.h"

#include "types/Vector.h"
#include "types/Color.h"

#ifndef M_MIN
#define M_MIN(_a, _b) ((_a)<(_b)?(_a):(_b))
#endif

Context::Context() {
}

Context::~Context() {
}

bool Context::load(const std::string& _filename) {
    config = YAML::LoadFile(_filename);

    // JS Globals
    JSValue global = parseNode(js, config["global"]);
    js.setGlobalValue("global", std::move(global));

    // Load MidiDevices
    std::vector<std::string> availableMidiOutPorts = MidiDevice::getOutPorts();

    // Define out targets
    if (config["out"].IsSequence()) {
        for (size_t i = 0; i < config["out"].size(); i++) {
            std::string name = config["out"][i].as<std::string>();
            Target target = parseTarget( name );

            if (target.protocol == MIDI_PROTOCOL) {
                MidiDevice* m = new MidiDevice(this, name);

                int deviceID = getMatchingKey(availableMidiOutPorts, target.address);
                if (deviceID >= 0)
                    m->openOutPort(target.address, deviceID);
                else
                    m->openVirtualOutPort(target.address);

                m->defaultOutChannel = toInt(target.port);

                if (target.folder == "/cc" || 
                    target.folder == "/control_change" )
                    m->defaultOutType = MidiDevice::CONTROLLER_CHANGE;
                else if (target.folder == "/note" ||
                            target.folder == "/note_on") 
                    m->defaultOutType = MidiDevice::NOTE_ON;
                else if (target.folder == "/note_off") 
                    m->defaultOutType = MidiDevice::NOTE_OFF;

                targetsDevicesNames.push_back( target.address );
                targetsDevices[target.address] = (Device*)m;
            }
            
            targets.push_back(target);
        }
    }

    // JS Functions
    uint32_t id = 0;
    
    // Load MidiDevices
    std::vector<std::string> availableMidiInPorts = MidiDevice::getInPorts();

    if (config["in"].IsMap()) {
        for (YAML::const_iterator dev = config["in"].begin(); dev != config["in"].end(); ++dev) {
            std::string inName = dev->first.as<std::string>();
            int deviceID = getMatchingKey(availableMidiInPorts, inName);

            if (deviceID >= 0) {
                MidiDevice* m = new MidiDevice(this, inName, deviceID);
                listenDevicesNames.push_back(inName);
                listenDevices[inName] = (Device*)m;

                for (size_t i = 0; i < config["in"][inName].size(); i++) {

                    if (config["in"][inName][i]["key"].IsDefined()) {

                        size_t channel = 0;
                        if (config["in"][inName][i]["channel"].IsDefined())
                            channel = config["in"][inName][i]["channel"].as<int>();
                        
                        bool haveShapingFunction = false;
                        if (config["in"][inName][i]["shape"].IsDefined()) {
                            std::string function = config["in"][inName][i]["shape"].as<std::string>();
                            if ( js.setFunction(id, function) ) {
                                haveShapingFunction = true;
                            }
                        }


                        if (config["in"][inName][i]["key"].IsScalar()) {
                            size_t key = config["in"][inName][i]["key"].as<size_t>();
                            // std::cout << " adding key " << key << std::endl;

                            listenDevices[inName]->setFncKey(channel, key, i);
                            // std::cout << " linking " << (inName + "_" + toString(key)) << " w id " << id << std::endl; 
                            if (haveShapingFunction)
                                shapeFncs[inName + "_" + toString(channel) + "_" + toString(key)] = id;
                        }
                        else if (config["in"][inName][i]["key"].IsSequence()) {
                            for (size_t j = 0; j < config["in"][inName][i]["key"].size(); j++) {
                                size_t key = config["in"][inName][i]["key"][j].as<size_t>();
                                // std::cout << " adding key " << key << std::endl;

                                listenDevices[inName]->setFncKey(channel, key, i);
                                // std::cout << " linking " << (inName + "_" + toString(key)) << " w id " << id << std::endl; 
                                if (haveShapingFunction)
                                    shapeFncs[inName + "_" + toString(channel) + "_" + toString(key)] = id;
                            }
                        }
                            
                        if (haveShapingFunction)
                            id++;
                    }   
                }
            
                updateDevice(inName);
            }
        }
    }

    if (listenDevices.size() == 0) {
        std::cout << "Heads up: MidiGyver is not listening to any of the available MIDI devices: " << std::endl;
        for (size_t i = 0; i < availableMidiInPorts.size(); i++)
            std::cout << "  - " << availableMidiInPorts[i] << std::endl;
    }

    // Load Pulses
    if (config["pulse"].IsSequence()) {
        for (size_t i = 0; i < config["pulse"].size(); i++) {
            YAML::Node n = config["pulse"][i];
            std::string name = n["name"].as<std::string>();

            // std::cout << "Adding pulse: " << name << std::endl;
            Pulse* p = new Pulse(this, i);
            
            if (n["bpm"].IsDefined())
                p->start(60000/n["bpm"].as<int>());
            else if (n["fps"].IsDefined()) 
                p->start(1000/int(n["fps"].as<float>()) );
            else if (n["interval"].IsDefined()) 
                p->start(int(n["interval"].as<float>()));

            if (n["shape"].IsDefined()) {
                std::string function = n["shape"].as<std::string>();
                if ( js.setFunction(id, function) ) {
                    shapeFncs[name + "_0_0"] = id;
                    id++;
                }
            }

            listenDevicesNames.push_back(name);
            listenDevices[name] = (Device*)p;
        }
    }

    return true;
}

bool Context::save(const std::string& _filename) {
    YAML::Emitter out;
    out.SetIndent(4);
    out.SetSeqFormat(YAML::Flow);
    out << config;

    std::ofstream fout(_filename);
    fout << out.c_str();

    return true;
}

bool Context::close() {
    for (std::map<std::string, Device*>::iterator it = listenDevices.begin(); it != listenDevices.end(); it++) {
        if (it->second->type == DEVICE_MIDI) {
            delete ((MidiDevice*)it->second);
        }
        else if (it->second->type == DEVICE_PULSE) {
            ((Pulse*)it->second)->stop();
            delete ((Pulse*)it->second);
        }
    }
    
    listenDevices.clear();
    listenDevicesNames.clear();

    shapeFncs.clear();

    targets.clear();
    targetsDevices.clear();
    targetsDevicesNames.clear();
    
    config = YAML::Node();

    return true;
}

bool Context::updateDevice(const std::string& _device) {

    for (size_t i = 0; i < config["in"][_device].size(); i++) {
        size_t key = i;

        size_t channel = 0;

        if (config["in"][_device][i]["channel"].IsDefined())
            channel = config["in"][_device][i]["channel"].as<size_t>();

        if (config["in"][_device][i]["key"].IsDefined()) {
            if (config["in"][_device][i]["key"].IsScalar()) {
                size_t key = config["in"][_device][i]["key"].as<size_t>();
                updateKey(config["in"][_device][i], _device, channel, key);
            }
            else if (config["in"][_device][i]["key"].IsSequence()) {
                for (size_t j = 0; j < config["in"][_device][i]["key"].size(); j++) {
                    size_t key = config["in"][_device][i]["key"][j].as<size_t>();
                    updateKey(config["in"][_device][i], _device, channel, key);
                }
            }
        }
        else {
            updateKey(config["in"][_device][i], _device, channel, i);
        }
    }

    return true;
}

bool Context::doKeyExist(const std::string& _device, size_t _channel, size_t _key) {
    if ( !config["in"][_device].IsNull() ) {
        if (config["in"][_device].IsSequence()) {
            return listenDevices[_device]->isFncKey(_channel, _key); 
        }
    }
    return false;
}

YAML::Node  Context::getKeyNode(const std::string& _device, size_t _channel, size_t _key) {
    if (config["in"][_device].IsSequence()) {
        size_t i = listenDevices[_device]->getFncKey(_channel, _key);
        return config["in"][_device][i];
    }

    return YAML::Node();
}

DataType Context::getKeyDataType(YAML::Node _keynode) {
    if ( _keynode.IsDefined() ) {
        if ( _keynode["type"].IsDefined() ) {
            std::string typeString =  _keynode["type"].as<std::string>();

            if (typeString == "button")
                return TYPE_BUTTON;
            else if (typeString == "toggle")
                return TYPE_TOGGLE;

            else if (   typeString == "state" ||
                        typeString == "enum" ||
                        typeString == "strings" )
                return TYPE_STRING;

            else if (   typeString == "scalar" ||
                        typeString == "number" ||
                        typeString == "float" ||
                        typeString == "int" )
                return TYPE_NUMBER;
                
            else if (   typeString == "vec2" ||
                        typeString == "vec3" ||
                        typeString == "vector" )
                return TYPE_VECTOR;
            else if (   typeString == "vec4" ||
                        typeString == "color" )
                return TYPE_COLOR;

            else if (   typeString == "note" )
                return TYPE_MIDI_NOTE;
            else if (   typeString == "cc" )
                return TYPE_MIDI_CONTROLLER_CHANGE;
            else if (   typeString == "tick" )
                return TYPE_MIDI_TIMING_TICK;
        }
    }
    return TYPE_NUMBER;
}

std::string Context::getKeyName(YAML::Node _keynode) {
    if ( _keynode.IsDefined() ) {
        if ( _keynode["name"].IsDefined() ) {
            return _keynode["name"].as<std::string>();
        }
    }
    return "unknownName";
}

bool Context::shapeKeyValue(YAML::Node _keynode, 
                            const std::string& _device, const std::string& _type, 
                            size_t _channel, size_t _key, 
                            float* _value) {

    if ( _keynode["shape"].IsDefined() ) {
        js.setGlobalValue("device", js.newString(_device));
        js.setGlobalValue("type", js.newString(_type));
        js.setGlobalValue("channel", js.newNumber(_channel));
        js.setGlobalValue("key", js.newNumber(_key));
        js.setGlobalValue("value", js.newNumber(*_value));

        JSValue keyData = parseNode(js, _keynode);
        js.setGlobalValue("data", std::move(keyData));

        std::string channel = toString( (size_t)_channel );        
        std::string key = toString(_key);
        std::string fnc = _device + "_" + channel + "_" + key;
        // std::cout << fnc << std::endl;
        JSValue result = js.getFunctionResult( shapeFncs[fnc] );
    
        if (!result.isNull()) {
            if (result.isString()) {
                JSScopeMarker marker1 = js.getScopeMarker();
                std::cout << "Update result on string: " << result.toString() << " but don't know what to do with it"<< std::endl;
                js.resetToScopeMarker(marker1);
                return false;
            }

            else if (result.isObject()) {
                JSScopeMarker marker1 = js.getScopeMarker();
                
                for (size_t j = 0; j < targetsDevicesNames.size(); j++) {
                    JSValue d = result.getValueForProperty(targetsDevicesNames[j]);
                    if (!d.isUndefined()) {
                        JSScopeMarker marker2 = js.getScopeMarker();

                        for (size_t i = 0; i < d.getLength(); i++) {
                            JSValue el = d.getValueAtIndex(i);
                            if (el.isArray()) {
                                if (el.getLength() > 1) {
                                    JSScopeMarker marker3 = js.getScopeMarker();

                                    MidiDevice* d = (MidiDevice*)targetsDevices[ targetsDevicesNames[j] ];
                                    size_t k = el.getValueAtIndex(0).toInt();
                                    size_t v = el.getValueAtIndex(1).toInt();
                                    d->send( k, v );

                                    js.resetToScopeMarker(marker3);
                                }
                            }
                        }

                        js.resetToScopeMarker(marker2);
                    }
                }
                
                for (size_t j = 0; j < listenDevicesNames.size(); j++) {
                    JSValue d = result.getValueForProperty(listenDevicesNames[j]);

                    if (!d.isUndefined()) {
                        JSScopeMarker marker2 = js.getScopeMarker();

                        for (size_t i = 0; i < d.getLength(); i++) {
                            JSValue el = d.getValueAtIndex(i);
                            if (el.isArray()) {
                                if (el.getLength() == 2) {
                                    JSScopeMarker marker3 = js.getScopeMarker();

                                    size_t k = el.getValueAtIndex(0).toInt();
                                    size_t v = el.getValueAtIndex(1).toInt();
                                    YAML::Node n = getKeyNode(listenDevicesNames[j], 0, k);
                                    mapKeyValue(n, listenDevicesNames[j], 0, k, v);

                                    js.resetToScopeMarker(marker3);
                                }
                                else if (el.getLength() == 3) {
                                    JSScopeMarker marker3 = js.getScopeMarker();

                                    size_t c = el.getValueAtIndex(0).toInt();
                                    size_t k = el.getValueAtIndex(1).toInt();
                                    float v = el.getValueAtIndex(2).toFloat();

                                    YAML::Node n = getKeyNode(listenDevicesNames[j], c, k);
                                    mapKeyValue(n, listenDevicesNames[j], c, k, v);

                                    js.resetToScopeMarker(marker3);
                                }
                            }
                        }

                        js.resetToScopeMarker(marker2);
                    }


                    JSValue d_leds = result.getValueForProperty(listenDevicesNames[j] + "_FEEDBACK");
                    if (!d_leds.isUndefined()) {
                        JSScopeMarker marker2 = js.getScopeMarker();

                        for (size_t i = 0; i < d_leds.getLength(); i++) {
                            JSValue el = d_leds.getValueAtIndex(i);

                            if (el.isArray()) {
                                if (el.getLength() == 2) {
                                    JSScopeMarker marker3 = js.getScopeMarker();

                                    size_t k = el.getValueAtIndex(0).toInt();
                                    YAML::Node n = getKeyNode(listenDevicesNames[j], 0, k);
                                    DataType n_type = getKeyDataType(n);
                                    size_t v = el.getValueAtIndex(1).toInt();
                                    feedbackLED(listenDevicesNames[j], 0, k, v);

                                    js.resetToScopeMarker(marker3);
                                }
                                else if (el.getLength() == 3) {
                                    JSScopeMarker marker3 = js.getScopeMarker();

                                    size_t c = el.getValueAtIndex(0).toInt();
                                    size_t k = el.getValueAtIndex(1).toInt();
                                    YAML::Node n = getKeyNode(listenDevicesNames[j], c, k);
                                    DataType n_type = getKeyDataType(n);
                                    float v = el.getValueAtIndex(2).toFloat();
                                    feedbackLED(listenDevicesNames[j], c, k, v);

                                    js.resetToScopeMarker(marker3);
                                }
                            }
                        }

                        js.resetToScopeMarker(marker2);
                    }                 
                }

                js.resetToScopeMarker(marker1);
                return false;
            }

            else if (result.isArray()) {
                JSScopeMarker marker1 = js.getScopeMarker();

                for (size_t i = 0; i < result.getLength(); i++) {
                    JSValue el = result.getValueAtIndex(i);
                    if (el.isArray()) {
                        if (el.getLength() == 2) {
                            size_t k = el.getValueAtIndex(0).toInt();
                            float v = el.getValueAtIndex(1).toFloat();
                            mapKeyValue(_keynode, _device, 0, k, v);
                        }
                        else if (el.getLength() == 3) {
                            size_t c = el.getValueAtIndex(0).toInt();
                            size_t k = el.getValueAtIndex(1).toInt();
                            float v = el.getValueAtIndex(2).toFloat();
                            mapKeyValue(_keynode, _device, c, k, v);
                        }
                        
                    }
                }

                js.resetToScopeMarker(marker1);
                return false;
            }
            
            else if (result.isNumber()) {
                *_value = result.toFloat();
            }

            else if (result.isBoolean()) {
                return result.toBool();
            }
        }
    }

    return true;
}

bool Context::mapKeyValue(YAML::Node _keynode, const std::string& _device, size_t _channel, size_t _key, float _value) {

    DataType type = getKeyDataType(_keynode);
    _keynode["value_raw"] = _value;

    // BUTTON
    if (type == TYPE_BUTTON) {
        bool value = _value > 0;
        _keynode["value"] = value;
        return updateKey(_keynode, _device, _channel, _key);
    }
    
    // TOGGLE
    else if ( type == TYPE_TOGGLE ) {
        if (_value > 0) {
            bool value = false;
                
            if (_keynode["value"])
                value = _keynode["value"].as<bool>();

            _keynode["value"] = !value;
            return updateKey(_keynode, _device, _channel, _key);
        }
    }

    // STATE
    else if ( type == TYPE_STRING ) {
        int value = (int)_value;
        std::string value_str = toString(value);

        if ( _keynode["map"] ) {
            if ( _keynode["map"].IsSequence() ) {
                float total = _keynode["map"].size();

                if (value == 127.0f) {
                    value_str = _keynode["map"][total-1].as<std::string>();
                } 
                else {
                    size_t index = (value / 127.0f) * _keynode["map"].size();
                    value_str = _keynode["map"][index].as<std::string>();
                } 
            }
        }
            
        _keynode["value"] = value_str;
        return updateKey(_keynode, _device, _channel, _key);
    }
    
    // SCALAR
    else if ( type == TYPE_NUMBER ) {
        float value = _value;

        if ( _keynode["map"] ) {
            value /= 127.0f;
            if ( _keynode["map"].IsSequence() ) {
                if ( _keynode["map"].size() > 1 ) {
                    float total = _keynode["map"].size() - 1;

                    size_t i_low = value * total;
                    size_t i_high = M_MIN(i_low + 1, size_t(total));
                    float pct = (value * total) - (float)i_low;
                    value = lerp(   _keynode["map"][i_low].as<float>(),
                                    _keynode["map"][i_high].as<float>(),
                                    pct );
                }
            }
        }
            
        _keynode["value"] = value;
        return updateKey(_keynode, _device, _channel, _key);
    }
    
    // VECTOR
    else if ( type == TYPE_VECTOR ) {
        float pct = _value / 127.0f;
        Vector value = Vector(0.0, 0.0, 0.0);

        if ( _keynode["map"] ) {
            if ( _keynode["map"].IsSequence() ) {
                if ( _keynode["map"].size() > 1 ) {
                    float total = _keynode["map"].size() - 1;

                    size_t i_low = pct * total;
                    size_t i_high = M_MIN(i_low + 1, size_t(total));

                    value = lerp(   _keynode["map"][i_low].as<Vector>(),
                                    _keynode["map"][i_high].as<Vector>(),
                                    (pct * total) - (float)i_low );
                }
            }
        }
            
        _keynode["value"] = value;
        return updateKey(_keynode, _device, _channel, _key);
    }

    // COLOR
    else if ( type == TYPE_COLOR ) {
        float pct = _value / 127.0f;
        Color value = Color(0.0, 0.0, 0.0);

        if ( _keynode["map"] ) {
            if ( _keynode["map"].IsSequence() ) {
                if ( _keynode["map"].size() > 1 ) {
                    float total = _keynode["map"].size() - 1;

                    size_t i_low = pct * total;
                    size_t i_high = M_MIN(i_low + 1, size_t(total));

                    value = lerp(   _keynode["map"][i_low].as<Color>(),
                                    _keynode["map"][i_high].as<Color>(),
                                    (pct * total) - (float)i_low );
                }
            }
        }
        
        _keynode["value"] = value;
        return updateKey(_keynode, _device, _channel, _key);
    }

    else if (   type == TYPE_MIDI_NOTE || 
                type == TYPE_MIDI_CONTROLLER_CHANGE ||
                type == TYPE_MIDI_TIMING_TICK ) {

        _keynode["value"] = int(_value);

        return updateKey(_keynode, _device, _channel, _key);
    }


    return false;
}

bool Context::updateKey(YAML::Node _keynode, const std::string& _device, size_t _channel, size_t _key) {

    if ( _keynode["value"].IsDefined() ) {

        // DataType type = getKeyDataType(_keynode);

        // // BUTTONs and TOGGLEs need to change state on the device
        // if (listenDevices[_device]->type == DEVICE_MIDI && 
        //     (type == TYPE_BUTTON || type == TYPE_TOGGLE)) {
        //     feedbackLED(_device, _key, _keynode["value"].as<bool>() ? 127 : 0);
        // }

        return sendKeyValue(_keynode, _device, _channel, _key);
    }
    
    return false;
}

bool Context::feedbackLED(const std::string& _device, size_t _channel, size_t _key, size_t _value){
    MidiDevice* midi = static_cast<MidiDevice*>(listenDevices[_device]);
    midi->send( MidiDevice::CONTROLLER_CHANGE, _channel, _key, _value);
    return true;
}

bool Context::sendKeyValue(YAML::Node _keynode, const std::string& _device, size_t _channel, size_t _key) {

    if ( !_keynode["value"].IsDefined() )
        return false;

    // Define out targets
    std::vector<Target> keyTargets;
    if (_keynode["out"].IsDefined() ) {
        if (_keynode["out"].IsSequence()) 
            for (size_t i = 0; i < _keynode["out"].size(); i++) {
                Target target = parseTarget( _keynode["out"][i].as<std::string>() ); 
                keyTargets.push_back( target );
            }
        else if (_keynode["out"].IsScalar()) {
            Target target = parseTarget( _keynode["out"].as<std::string>() );
            keyTargets.push_back( target );
        }
    }
    else
        keyTargets = targets;
        
    // KEY
    std::string name = "unknown"; 
    // std::string name = getKeyName(_keynode);
    if ( _keynode.IsDefined() ) {
        if ( _keynode["name"].IsDefined() ) {
            name = _keynode["name"].as<std::string>();
        }
        else {
            name = ""; 
            if ( _keynode["channel"].IsDefined() )
                name += toString(_channel) + ",";
            name += toString(_key);
        } 
    }

    DataType type = getKeyDataType(_keynode);
    YAML::Node value = _keynode["value"];

    // BUTTON and TOGGLE
    if ( type == TYPE_TOGGLE || type == TYPE_BUTTON ) {
        std::string value_str = (value.as<bool>()) ? "on" : "off";
        
        if (_keynode["map"]) {

            if (_keynode["map"][value_str]) {

                // If the end mapped string is a sequence
                if (_keynode["map"][value_str].IsSequence()) {
                    for (size_t i = 0; i < _keynode["map"][value_str].size(); i++) {
                        std::string prop = name;
                        std::string msg = "";

                        if ( parseString(_keynode["map"][value_str][i], prop, msg) ) {
                            for (size_t t = 0; t < keyTargets.size(); t++)
                                broadcast(keyTargets[t], prop, msg);
                        }

                    }
                }
                else {
                    std::string prop = name;
                    std::string msg = "";

                    if ( parseString(_keynode["map"][value_str], prop, msg) ) {
                        for (size_t t = 0; t < keyTargets.size(); t++)
                            broadcast(keyTargets[t], prop, msg);
                    }
                }

            }

        }
        else {
            for (size_t t = 0; t < keyTargets.size(); t++)
                broadcast(keyTargets[t], name, value_str);

        }

        if ( listenDevices[_device]->type == DEVICE_MIDI )
            feedbackLED(_device, _channel, _key, _keynode["value"].as<bool>() ? 127 : 0);

        return true;
    }

    // STATE
    else if ( type == TYPE_STRING ) {
        for (size_t t = 0; t < keyTargets.size(); t++)
            broadcast(keyTargets[t], name, value.as<std::string>());

        return true;
    }

    // SCALAR
    else if ( type == TYPE_NUMBER ) {
        for (size_t t = 0; t < keyTargets.size(); t++)
            broadcast(keyTargets[t], name, value.as<float>());

        return true;
    }

    // VECTOR
    else if ( type == TYPE_VECTOR ) {
        for (size_t t = 0; t < keyTargets.size(); t++)
            broadcast(keyTargets[t], name, value.as<Vector>());

        return true;
    }

    // COLOR
    else if ( type == TYPE_COLOR ) {
        for (size_t t = 0; t < keyTargets.size(); t++)
            broadcast(keyTargets[t], name, value.as<Color>());
        
        return true;
    }
    else if (targetsDevices.size() > 0) {
        size_t value = _keynode["value"].as<int>();

        for (size_t t = 0; t < keyTargets.size(); t++) {
            if (keyTargets[t].protocol == MIDI_PROTOCOL) {
                std::string name = keyTargets[t].address;

                if ( targetsDevices.find( name ) == targetsDevices.end() ) {
                    MidiDevice* d = (MidiDevice*)targetsDevices[ name ];

                    if ( type == TYPE_MIDI_NOTE) {
                        if (value == 0)
                            d->send( MidiDevice::NOTE_OFF, _key, 0 );
                        else 
                            d->send( MidiDevice::NOTE_ON, _key, value );
                    }
                    else if ( type == TYPE_MIDI_CONTROLLER_CHANGE ) {
                        d->send( MidiDevice::CONTROLLER_CHANGE, _key, value );
                    }
                    else if ( type == TYPE_MIDI_TIMING_TICK ) {
                        d->send( MidiDevice::TIMING_TICK );
                    }
                }
            }
        }
    }

    return false;
}


