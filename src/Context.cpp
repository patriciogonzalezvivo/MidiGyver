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

DataType toDataType(const std::string& _string ) {
    std::string typeString = toLower(_string);

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

    else if (   typeString == "note" || 
                typeString == "note_on" )
        return TYPE_MIDI_NOTE;

    else if (   typeString == "cc" ||
                typeString == "controller_change" )
        return TYPE_MIDI_CONTROLLER_CHANGE;

    else if (   typeString == "tick" ||
                typeString == "timing_tick" )
        return TYPE_MIDI_TIMING_TICK;

    return TYPE_UNKNOWN;
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

                if (target.folder != "/")
                    m->defaultOutStatus = MidiDevice::statusNameToByte( target.folder.erase(0, 1) );

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

                            listenDevices[inName]->setFncKey(channel, key, i);
                            if (haveShapingFunction)
                                shapeFncs[inName + "_" + toString(channel) + "_" + toString(key)] = id;
                        }
                        else if (config["in"][inName][i]["key"].IsSequence()) {
                            for (size_t j = 0; j < config["in"][inName][i]["key"].size(); j++) {
                                size_t key = config["in"][inName][i]["key"][j].as<size_t>();

                                listenDevices[inName]->setFncKey(channel, key, i);
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

            Pulse* p = new Pulse(this, i);
            
            if (n["channel"].IsDefined())
                p->defaultOutChannel = n["channel"].as<int>();

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
        unsigned char status = MidiDevice::CONTROLLER_CHANGE;
        size_t channel = 0;
        size_t key = i;

        if (config["in"][_device][i]["channel"].IsDefined())
            channel = config["in"][_device][i]["channel"].as<size_t>();

        if (config["in"][_device][i]["key"].IsDefined()) {
            if (config["in"][_device][i]["key"].IsScalar()) {
                size_t key = config["in"][_device][i]["key"].as<size_t>();
                updateKey(config["in"][_device][i], _device, status, channel, key);
            }
            else if (config["in"][_device][i]["key"].IsSequence()) {
                for (size_t j = 0; j < config["in"][_device][i]["key"].size(); j++) {
                    size_t key = config["in"][_device][i]["key"][j].as<size_t>();
                    updateKey(config["in"][_device][i], _device, status, channel, key);
                }
            }
        }
        else {
            updateKey(config["in"][_device][i], _device, status, channel, i);
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
            return toDataType( _keynode["type"].as<std::string>() );
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
                            const std::string& _device, unsigned char _status, size_t _channel,
                            size_t _key, float* _value) {

    if ( _keynode["shape"].IsDefined() ) {

        size_t channel = _channel;

        std::string status = MidiDevice::statusByteToName(_status);

        if ( !_keynode["channel"].IsDefined() )
            channel = 0;

        js.setGlobalValue("device", js.newString(_device));
        js.setGlobalValue("status", js.newString( status ));
        js.setGlobalValue("channel", js.newNumber(channel));
        js.setGlobalValue("key", js.newNumber(_key));
        js.setGlobalValue("value", js.newNumber(*_value));

        JSValue keyData = parseNode(js, _keynode);
        js.setGlobalValue("data", std::move(keyData));

        JSValue result = js.getFunctionResult( shapeFncs[ _device + "_" + toString( (size_t)channel ) + "_" + toString(_key) ] );
    
        if (!result.isNull()) {

            // Result is a string
            if (result.isString()) {
                JSScopeMarker marker1 = js.getScopeMarker();
                std::cout << "Update result on string: " << result.toString() << " but don't know what to do with it"<< std::endl;
                js.resetToScopeMarker(marker1);
                return false;
            }

            // Result is an object
            else if (result.isObject()) {
                JSScopeMarker marker1 = js.getScopeMarker();
                
                // Check on all target devices
                for (size_t j = 0; j < targetsDevicesNames.size(); j++) {

                    // on the same status
                    JSValue d = result.getValueForProperty( targetsDevicesNames[j] );
                    if (!d.isUndefined()) {
                        JSScopeMarker marker2 = js.getScopeMarker();

                        for (size_t i = 0; i < d.getLength(); i++) {
                            JSValue el = d.getValueAtIndex(i);
                            if (el.isArray()) {
                                if (el.getLength() > 1) {
                                    JSScopeMarker marker3 = js.getScopeMarker();

                                    MidiDevice* t = (MidiDevice*)targetsDevices[ targetsDevicesNames[j] ];
                                    size_t k = el.getValueAtIndex(0).toInt();
                                    size_t v = el.getValueAtIndex(1).toInt();
                                    t->trigger(t->defaultOutStatus, 0, k, v );

                                    js.resetToScopeMarker(marker3);
                                }
                            }
                        }

                        js.resetToScopeMarker(marker2);
                    }

                    for (size_t s = 0; s < 3; s++) {
                        char unsigned sByte = MidiDevice::getStatusByte(s);
                        std::string sName = MidiDevice::getStatusName(s);

                        // on the same status
                        JSValue d2 = result.getValueForProperty( targetsDevicesNames[j] + "/" + sName);
                        if (!d2.isUndefined()) {
                            JSScopeMarker marker2 = js.getScopeMarker();

                            for (size_t i = 0; i < d2.getLength(); i++) {
                                JSValue el = d2.getValueAtIndex(i);
                                if (el.isArray()) {
                                    if (el.getLength() > 1) {
                                        JSScopeMarker marker3 = js.getScopeMarker();

                                        MidiDevice* t = (MidiDevice*)targetsDevices[ targetsDevicesNames[j] ];
                                        size_t k = el.getValueAtIndex(0).toInt();
                                        size_t v = el.getValueAtIndex(1).toInt();
                                        t->trigger(sByte, 0, k, v );

                                        js.resetToScopeMarker(marker3);
                                    }
                                }
                            }

                            js.resetToScopeMarker(marker2);
                        }
                    }

                }
                
                for (size_t j = 0; j < listenDevicesNames.size(); j++) {

                    // RETURN the same status as recieved
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
                                    mapKeyValue(n, listenDevicesNames[j], _status, 0, k, v);

                                    js.resetToScopeMarker(marker3);
                                }
                                else if (el.getLength() == 3) {
                                    JSScopeMarker marker3 = js.getScopeMarker();

                                    size_t c = el.getValueAtIndex(0).toInt();
                                    size_t k = el.getValueAtIndex(1).toInt();
                                    float v = el.getValueAtIndex(2).toFloat();

                                    YAML::Node n = getKeyNode(listenDevicesNames[j], c, k);
                                    mapKeyValue(n, listenDevicesNames[j], _status, c, k, v);

                                    js.resetToScopeMarker(marker3);
                                }
                            }
                        }

                        js.resetToScopeMarker(marker2);
                    }

                    JSValue d_leds = result.getValueForProperty(listenDevicesNames[j] + "/CONTROLLER_CHANGE");
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
                                    feedback(listenDevicesNames[j], MidiDevice::CONTROLLER_CHANGE, 0, k, v);

                                    js.resetToScopeMarker(marker3);
                                }
                                else if (el.getLength() == 3) {
                                    JSScopeMarker marker3 = js.getScopeMarker();

                                    size_t c = el.getValueAtIndex(0).toInt();
                                    size_t k = el.getValueAtIndex(1).toInt();
                                    YAML::Node n = getKeyNode(listenDevicesNames[j], c, k);
                                    DataType n_type = getKeyDataType(n);
                                    float v = el.getValueAtIndex(2).toFloat();
                                    feedback(listenDevicesNames[j], MidiDevice::CONTROLLER_CHANGE, c, k, v);

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

            // Result is an array
            else if (result.isArray()) {
                JSScopeMarker marker1 = js.getScopeMarker();

                for (size_t i = 0; i < result.getLength(); i++) {
                    JSValue el = result.getValueAtIndex(i);
                    if (el.isArray()) {
                        if (el.getLength() == 2) {
                            size_t k = el.getValueAtIndex(0).toInt();
                            float v = el.getValueAtIndex(1).toFloat();
                            mapKeyValue(_keynode, _device, _status, 0, k, v);
                        }
                        else if (el.getLength() == 3) {
                            size_t c = el.getValueAtIndex(0).toInt();
                            size_t k = el.getValueAtIndex(1).toInt();
                            float v = el.getValueAtIndex(2).toFloat();
                            mapKeyValue(_keynode, _device, _status, c, k, v);
                        }
                        
                    }
                }

                js.resetToScopeMarker(marker1);
                return false;
            }
            
            // Result is a number
            else if (result.isNumber()) {
                *_value = result.toFloat();
            }

            // Result is a boolean
            else if (result.isBoolean()) {
                return result.toBool();
            }
        }
    }

    return true;
}

bool Context::mapKeyValue(  YAML::Node _keynode, 
                            const std::string& _device, unsigned char _status, size_t _channel, 
                            size_t _key, float _value) {

    DataType type = getKeyDataType(_keynode);
    _keynode["value_raw"] = _value;

    // BUTTON
    if (type == TYPE_BUTTON) {
        bool value = _value > 0;
        _keynode["value"] = value;
        return updateKey(_keynode, _device, _status, _channel, _key);
    }
    
    // TOGGLE
    else if ( type == TYPE_TOGGLE ) {
        if (_value > 0) {
            bool value = false;
                
            if (_keynode["value"])
                value = _keynode["value"].as<bool>();

            _keynode["value"] = !value;
            return updateKey(_keynode, _device, _status, _channel, _key);
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
        return updateKey(_keynode, _device, _status, _channel, _key);
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
        return updateKey(_keynode, _device, _status, _channel, _key);
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
        return updateKey(_keynode, _device, _status, _channel, _key);
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
        return updateKey(_keynode, _device, _status, _channel, _key);
    }

    else if (   type == TYPE_MIDI_NOTE || 
                type == TYPE_MIDI_CONTROLLER_CHANGE ||
                type == TYPE_MIDI_TIMING_TICK ) {

        _keynode["value"] = int(_value);
        return updateKey(_keynode, _device, _status, _channel, _key);
    }

    return false;
}

std::vector<Target> Context::getTargetsForNode(YAML::Node _keynode) {
    if (_keynode["out"].IsDefined() ) {
        std::vector<Target> keyTargets;
        if (_keynode["out"].IsSequence()) 
            for (size_t i = 0; i < _keynode["out"].size(); i++) {
                keyTargets.push_back( parseTarget( _keynode["out"][i].as<std::string>() ) );
            }
        else if (_keynode["out"].IsScalar()) {
            Target target = parseTarget( _keynode["out"].as<std::string>() );
            keyTargets.push_back( target );
        }
        return keyTargets;
    }
    else
        return targets;
}


bool Context::updateKey(YAML::Node _keynode, 
                        const std::string& _device, unsigned char _status, size_t _channel, 
                        size_t _key) {

    if ( !_keynode["value"].IsDefined() )
        return false;

    // Define out targets
    std::vector<Target> keyTargets = getTargetsForNode(_keynode);
        
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
            feedback(_device, _status, _channel, _key, _keynode["value"].as<bool>() ? 127 : 0);

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
                            d->trigger( MidiDevice::NOTE_OFF, 0, _key, 0 );
                        else 
                            d->trigger( MidiDevice::NOTE_ON, 0, _key, value );
                    }
                    else if ( type == TYPE_MIDI_CONTROLLER_CHANGE )
                        d->trigger( MidiDevice::CONTROLLER_CHANGE, 0, _key, value );
                    
                    else if ( type == TYPE_MIDI_TIMING_TICK )
                        d->trigger( MidiDevice::TIMING_TICK, 0 );
                }
            }
        }
    }

    return false;
}

bool Context::feedback(const std::string& _device, unsigned char _status, size_t _channel, size_t _key, size_t _value) {
    MidiDevice* midi = static_cast<MidiDevice*>(listenDevices[_device]);
    midi->trigger( _status, _channel, _key, _value);
    return true;
}


