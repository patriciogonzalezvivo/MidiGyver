#include "Context.h"

#include "ops/broadcast.h"

#include "ops/nodes.h"
#include "ops/strings.h"
#include "ops/times.h"

#include "types/Vector.h"
#include "types/Color.h"

#ifndef M_MIN
#define M_MIN(_a, _b) ((_a)<(_b)?(_a):(_b))
#endif

Context::Context() : tickDuration(250), safe(false) {
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

    // Define global tick
    if (config["global"].IsDefined()) 
        getTickDuration(config["global"], &tickDuration);

    // Define out targets
    if (config["out"].IsSequence()) {
        for (size_t i = 0; i < config["out"].size(); i++) {
            std::string name = config["out"][i].as<std::string>();
            Address target = parseAddress( name );

            if (target.protocol == MIDI_PROTOCOL) {
                if (target.isFile) {
                    if (targetsTerm.find(target.address) == targetsTerm.end()) {
                        MidiFile* m = new MidiFile(this, target.address);
                        targetsTerm[target.address] = (Term*)m;
                        target.term = (Term*)m;
                    }
                }
                else {
                    MidiDevice* m = new MidiDevice(this, name);

                    int deviceID = getMatchingKey(availableMidiOutPorts, target.address);
                    if (deviceID >= 0)
                        m->openOutPort(target.address, deviceID);
                    else
                        m->openVirtualOutPort(target.address);

                    targetsTerm[target.address] = (Term*)m;
                    target.term = (Term*)m;
                }
            }
            
            targets.push_back(target);
        }
    }

    // Reset JS ID counter
    js.idCounter = 0;
    // Load MidiDevices
    if (config["in"].IsMap()) {

        for (YAML::const_iterator dev = config["in"].begin(); dev != config["in"].end(); ++dev) {
            std::string nName = dev->first.as<std::string>();
            std::string dName = nName;

            Address src = parseAddress(nName);

            if ((src.protocol == UNKNOWN_PROTOCOL && !src.isFile) || 
                (src.protocol == MIDI_PROTOCOL && !src.isFile) ) {
                if (src.protocol == MIDI_PROTOCOL)
                    dName = src.address;
                
                src.term = loadMidiDeviceIn(dName, config["in"][nName]);

            }
            else if (src.protocol == OSC_PROTOCOL) {
                src.term = loadOscDeviceIn(src.address, toInt(src.port), config["in"][nName]);
            }

            sources.push_back(src);
            // targets.push_back(src); // for feedback
        }
    }

    if (sourcesTerm.size() == 0) {
        std::vector<std::string> availableMidiInPorts = MidiDevice::getInPorts();
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
            
            int tick = 250; // 120bpm
            if (!getTickDuration(n, &tick))
                tick = tickDuration;

            p->start(tick);


            if (n["shape"].IsDefined()) {
                std::string function = n["shape"].as<std::string>();
                if ( js.setFunction(js.idCounter, function) ) {
                    shapeFncs[name + "_TIMING_TICK"] = js.idCounter;
                    js.idCounter++;
                }
            }
            sourcesTerm[name] = (Device*)p;
        }
    }

    safe = true;
    return safe;
}

Term* Context::loadMidiDeviceIn(const std::string& _devicesName, YAML::Node _node) {
    std::vector<std::string> availableMidiInPorts = MidiDevice::getInPorts();

    int id = getMatchingKey(availableMidiInPorts, _devicesName);
    if (id >= 0) {
        MidiDevice* m = new MidiDevice(this, _devicesName, id);
        // sourcesTermNames.push_back(_devicesName);
        sourcesTerm[_devicesName] = (Device*)m;
        m->node = _node;
    
        for (size_t i = 0; i < _node.size(); i++) {

            // ADD KEY EVENT
            if (_node[i]["key"].IsDefined()) {

                size_t channel = 0;
                if (_node[i]["channel"].IsDefined())
                    channel = _node[i]["channel"].as<int>();
                
                bool haveShapingFunction = false;
                if (_node[i]["shape"].IsDefined()) {
                    std::string function = _node[i]["shape"].as<std::string>();
                    if ( js.setFunction(js.idCounter, function) ) {
                        haveShapingFunction = true;
                    }
                }

                // Get matching key/s
                std::vector<size_t> keys = getArrayOfKeys(_node[i]["key"]);

                // if it's only one 
                if (keys.size() == 1) {
                    size_t key = keys[0];
                    _node[i]["key"] = key;
                    m->setKeyFnc(channel, key, i);
                        if (haveShapingFunction)
                            shapeFncs[_devicesName + "_" + toString(channel) + "_" + toString(key)] = js.idCounter;
                }
                // If they are multiple keys
                else if (keys.size() > 1) {
                    _node[i].remove("key");

                    for (size_t j = 0; j < keys.size(); j++) {
                        _node[i]["key"].push_back(keys[j]);
                        m->setKeyFnc(channel, keys[j], i);
                        if (haveShapingFunction)
                            shapeFncs[_devicesName + "_" + toString(channel) + "_" + toString(keys[j])] = js.idCounter;
                    }

                }

                if (haveShapingFunction)
                    js.idCounter++;
            }

            // ADD STATUS ONLY EVENT
            else if (_node[i]["status"].IsDefined()) {

                std::string status_str = _node[i]["status"].as<std::string>();
                status_str = toUpper(status_str);
                unsigned char status = Midi::statusNameToByte(status_str);

                if (status == Midi::TIMING_TICK ||
                    status == Midi::START_SONG ||
                    status == Midi::CONTINUE_SONG ||
                    status == Midi::STOP_SONG ) {

                    m->setStatusFnc(status, i);
                    if (_node[i]["shape"].IsDefined()) {
                        std::string function = _node[i]["shape"].as<std::string>();
                        if ( js.setFunction(js.idCounter, function) ) {
                            shapeFncs[_devicesName + "_" + status_str] = js.idCounter;
                            js.idCounter++;
                        }
                    }
                }
            }
        }

        updateDevice(_devicesName);

        return (Term*)m;
    }

    return nullptr;
}

Term* Context::loadOscDeviceIn(const std::string& _devicesName, size_t _port, YAML::Node _node) {
    
    OscDevice* d = new OscDevice(this, _devicesName, _port);
    d->node = _node;

    sourcesTerm[_devicesName] = (Device*)d;

    for (size_t i = 0; i < _node.size(); i++) {

        // ADD KEY EVENT
        if (_node[i]["key"].IsDefined()) {

            size_t channel = 0;
            if (_node[i]["channel"].IsDefined())
                channel = _node[i]["channel"].as<int>();
            
            bool haveShapingFunction = false;
            if (_node[i]["shape"].IsDefined()) {
                std::string function = _node[i]["shape"].as<std::string>();
                if ( js.setFunction(js.idCounter, function) ) {
                    haveShapingFunction = true;
                }
            }

            // Get matching key/s
            std::vector<size_t> keys = getArrayOfKeys(_node[i]["key"]);

            // if it's only one 
            if (keys.size() == 1) {
                size_t key = keys[0];
                _node[i]["key"] = key;
                d->setKeyFnc(channel, key, i);
                    if (haveShapingFunction)
                        shapeFncs[_devicesName + "_" + toString(channel) + "_" + toString(key)] = js.idCounter;
            }
            // If they are multiple keys
            else if (keys.size() > 1) {
                _node[i].remove("key");
                for (size_t j = 0; j < keys.size(); j++) {
                    _node[i]["key"].push_back(keys[j]);
                    d->setKeyFnc(channel, keys[j], i);
                    if (haveShapingFunction)
                        shapeFncs[_devicesName + "_" + toString(channel) + "_" + toString(keys[j])] = js.idCounter;
                }
            }

            if (haveShapingFunction)
                js.idCounter++;
        }

        // ADD STATUS ONLY EVENT
        else if (_node[i]["status"].IsDefined()) {

            std::string status_str = _node[i]["status"].as<std::string>();
            status_str = toUpper(status_str);
            unsigned char status = Midi::statusNameToByte(status_str);

            if (status == Midi::TIMING_TICK ||
                status == Midi::START_SONG ||
                status == Midi::CONTINUE_SONG ||
                status == Midi::STOP_SONG ) {

                d->setStatusFnc(status, i);
                if (_node[i]["shape"].IsDefined()) {
                    std::string function = _node[i]["shape"].as<std::string>();
                    if ( js.setFunction(js.idCounter, function) ) {
                        shapeFncs[_devicesName + "_" + status_str] = js.idCounter;
                        js.idCounter++;
                    }
                }
            }
        }
    }

    updateDevice(_devicesName);

    return (Term*)d;
}

void Context::updateDevice(const std::string& _name) {
    std::map<std::string, Term*>::iterator it = sourcesTerm.find(_name);
    
    if (it != sourcesTerm.end()) {
        Device* d = (Device*)it->second;

        for (size_t i = 0; i < d->node.size(); i++) {
            unsigned char status = Midi::CONTROLLER_CHANGE;
            size_t channel = 0;
            size_t key = i;

            if (d->node[i]["channel"].IsDefined())
                channel = d->node[i]["channel"].as<size_t>();

            // Key Nodes
            if (d->node[i]["key"].IsDefined()) {
                if (d->node[i]["key"].IsScalar()) {
                    size_t key = d->node[i]["key"].as<size_t>();
                    updateNode(d->node[i], _name, status, channel, key);
                }
                else if (d->node[i]["key"].IsSequence()) {
                    for (size_t j = 0; j < d->node[i]["key"].size(); j++) {
                        size_t key = d->node[i]["key"][j].as<size_t>();
                        updateNode(d->node[i], _name, status, channel, key);
                    }
                }
            }

            // // Status Nodes
            // else {
            // //     updateNode(config["in"][_term][i], _term, status, channel, i);
            // }
        }
    }
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
    safe = false;

    // close listen devices
    for (std::map<std::string, Term*>::iterator it = sourcesTerm.begin(); it != sourcesTerm.end(); it++) {
        if (it->second->getType() == MIDI_DEVICE) {
            delete ((MidiDevice*)it->second);
        }
        else if (it->second->getType() == OSC_DEVICE) {
            delete ((OscDevice*)it->second);
        }
        else if (it->second->getType() == PULSE) {
            ((Pulse*)it->second)->stop();
            delete ((Pulse*)it->second);
        }
    }

    // close target devices
    for (std::map<std::string, Term*>::iterator it = targetsTerm.begin(); it != targetsTerm.end(); it++) {
        if (it->second->getType() == PULSE) {
            ((Pulse*)it->second)->stop();
            delete ((Pulse*)it->second);
        }
        else if (it->second->getType() == MIDI_DEVICE) {
            delete ((MidiDevice*)it->second);
        }
        else if (it->second->getType() == MIDI_FILE) {
            ((MidiFile*)it->second)->close();
            delete ((MidiFile*)it->second);
        }
    }

    // Save Midi file
    sources.clear();
    sourcesTerm.clear();
    shapeFncs.clear();

    targets.clear();
    targetsTerm.clear();
    
    config = YAML::Node();

    return true;
}

bool Context::doStatusExist(const std::string& _term, unsigned char _status) {
    std::map<std::string, Term*>::iterator it = sourcesTerm.find(_term);
    if (it != sourcesTerm.end())
        if (it->second->getType() == MIDI_DEVICE || it->second->getType() == OSC_DEVICE )
            return ((Device*)it->second)->isStatusFnc(_status); 

    return false;
}

YAML::Node Context::getStatusNode(const std::string& _term, unsigned char _status) {
    std::map<std::string, Term*>::iterator it = sourcesTerm.find(_term);
    if (it != sourcesTerm.end())
        if (it->second->getType() == MIDI_DEVICE || it->second->getType() == OSC_DEVICE ) {
            size_t i = ((MidiDevice*)it->second)->getStatusFnc(_status);
            return ((MidiDevice*)it->second)->node[i];
        }

    return YAML::Node();
}

bool Context::doKeyExist(const std::string& _term, size_t _channel, size_t _key) {
    std::map<std::string, Term*>::iterator it = sourcesTerm.find(_term);
    if (it != sourcesTerm.end())
        if (it->second->getType() == MIDI_DEVICE || it->second->getType() == OSC_DEVICE )
            return ((Device*)it->second)->isKeyFnc(_channel, _key); 

    return false;
}

YAML::Node  Context::getKeyNode(const std::string& _term, size_t _channel, size_t _key) {
    std::map<std::string, Term*>::iterator it = sourcesTerm.find(_term);
    if (it != sourcesTerm.end())
        if (it->second->getType() == MIDI_DEVICE || it->second->getType() == OSC_DEVICE  ) {
            size_t i = ((MidiDevice*)it->second)->getKeyFnc(_channel, _key);
            return ((MidiDevice*)it->second)->node[i];
        }

    return YAML::Node();
}

DataType Context::getKeyDataType(YAML::Node _node) {
    if ( _node.IsDefined() ) {
        if ( _node["type"].IsDefined() ) {
            return toDataType( _node["type"].as<std::string>() );
        }
    }
    return TYPE_UNKNOWN;
}

bool Context::processEvent( YAML::Node _node, 
                            const std::string& _term, unsigned char _status, size_t _channel, 
                            size_t _key, float _value, bool _statusOnly) {

    if (shapeValue(_node, _term, _status, _channel, _key, &_value, _statusOnly))
        mapValue(_node, _term, _status, _channel, _key, _value);

    return true;
}

bool Context::shapeValue(YAML::Node _node, 
                            const std::string& _term, unsigned char _status, size_t _channel,
                            size_t _key, float* _value, bool _statusOnly) {

    AddressList keyTargets = getTargetsForNode(_node);

    if ( _node["shape"].IsDefined() ) {
        size_t channel = _channel;

        std::string status = Midi::statusByteToName(_status);

        if ( !_node["channel"].IsDefined() )
            channel = 0;

        std::string type = toString( getKeyDataType(_node) );
        if (type == "UNKNOWN")
            type = status;

        js.setGlobalValue("device", js.newString(_term));
        js.setGlobalValue("status", js.newString( status ));
        js.setGlobalValue("type", js.newString( type ));
        js.setGlobalValue("channel", js.newNumber( channel ));
        js.setGlobalValue("key", js.newNumber(_key));
        js.setGlobalValue("value", js.newNumber(*_value));

        if ( _node["value_raw"].IsDefined() )
            js.setGlobalValue("value_raw", js.newNumber( _node["value_raw"].as<float>() ) );
        else {
            js.setGlobalValue("value_raw", js.newNumber( *_value ) );
            _node["value_raw"] = *_value;
        }

        JSValue keyData = parseNode(js, _node);
        js.setGlobalValue("data", std::move(keyData));

        std::string fnc_index = _term + "_";
        if (_statusOnly)
            fnc_index += status;
        else
            fnc_index += toString( (size_t)channel ) + "_" + toString(_key);
        

        JSValue result = js.getFunctionResult( shapeFncs[ fnc_index ] );
    
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

                // Get Object keys
                std::vector<std::string> keys = result.getKeys();

                // Iterate through them
                for (size_t t = 0; t < keys.size(); t++) {
                    // Parse the keys into a target
                    Address target = parseAddress( keys[t] );

                    // std::cout << "key: " << keys[t] << std::endl;

                    // If the target is MIDI
                    if (target.protocol == MIDI_PROTOCOL) {
                        // and exist as a target
                        
                        // Get values
                        JSValue values = result.getValueForProperty( keys[t] );
                        if (!values.isUndefined()) {
                            JSScopeMarker marker2 = js.getScopeMarker();

                            // Iterate through elements
                            for (size_t i = 0; i < values.getLength(); i++) {
                                JSValue el = values.getValueAtIndex(i);
                                JSScopeMarker marker3 = js.getScopeMarker();

                                // if elements are arrays
                                if (el.isArray()) {

                                    //  with more than one elements
                                    if (el.getLength() > 1) {

                                        size_t c = toInt(target.port);
                                        size_t k = el.getValueAtIndex(0).toInt();
                                        size_t v = el.getValueAtIndex(1).toInt();
                                        unsigned char s = _status;

                                        if (target.folder != "/") 
                                            s = Midi::statusNameToByte(target.folder);

                                        if (el.getLength() == 3) {
                                            c = k;
                                            k = v;
                                            v = el.getValueAtIndex(2).toInt();
                                        }

                                        // std::cout << "shaping> " << Midi::statusByteToName(s) << " " << c << " " << k << " " << v << std::endl;

                                        // If it's a target
                                        std::map<std::string, Term*>::iterator it = targetsTerm.find(target.address);
                                        if (it != targetsTerm.end()) {
                                            if (target.isFile) {
                                                MidiFile* t = (MidiFile*)it->second;
                                                t->trigger(target.address, s, c, k, v );
                                            }
                                            else {
                                                MidiDevice* t = (MidiDevice*)it->second;
                                                t->trigger(s, c, k, v );
                                            }
                                        }

                                        // It it's also a source
                                        it = sourcesTerm.find(target.address);
                                        if (it != sourcesTerm.end()) {
                                            if (s == Midi::CONTROLLER_CHANGE && !target.isFile)
                                                ((MidiDevice*)it->second)->trigger(s, c, k, v);
                                                feedback(target.address, s, c, k, v);
                                            
                                            if (doKeyExist(target.address, c, k)) {
                                                YAML::Node n = getKeyNode(target.address, c, k);
                                                mapValue(n, target.address, _status, c, k, v);
                                            }
                                        }
                                        
                                    }
                                }
                                js.resetToScopeMarker(marker3);
                            }
                            js.resetToScopeMarker(marker2);
                        }

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
                            mapValue(_node, _term, _status, 0, k, v);
                        }
                        else if (el.getLength() == 3) {
                            size_t c = el.getValueAtIndex(0).toInt();
                            size_t k = el.getValueAtIndex(1).toInt();
                            float v = el.getValueAtIndex(2).toFloat();
                            mapValue(_node, _term, _status, c, k, v);
                        }
                        
                    }
                }

                js.resetToScopeMarker(marker1);
                return false;
            }
            
            // Result is a number
            else if (result.isNumber())
                *_value = result.toFloat();

            // Result is a boolean
            else if (result.isBoolean())
                return result.toBool();
        }
    }

    return true;
}

bool Context::mapValue(  YAML::Node _node, 
                            const std::string& _term, unsigned char _status, size_t _channel, 
                            size_t _key, float _value) {

    _node["value_raw"] = _value;
    DataType type = getKeyDataType(_node);
    
    // BUTTON
    if (type == TYPE_BUTTON) {
        bool value = _value > 0;
        _node["value"] = value;
        return updateNode(_node, _term, _status, _channel, _key);
    }
    
    // TOGGLE
    else if ( type == TYPE_TOGGLE ) {
        if (_value > 0) {
            bool value = false;
                
            if (_node["value"])
                value = _node["value"].as<bool>();

            _node["value"] = !value;
            return updateNode(_node, _term, _status, _channel, _key);
        }
    }

    // STATE
    else if ( type == TYPE_STRING ) {
        int value = (int)_value;
        std::string value_str = toString(value);

        if ( _node["map"] ) {
            if ( _node["map"].IsSequence() ) {
                float total = _node["map"].size();

                if (total == 1) {
                    value_str = _node["map"][0].as<std::string>();
                }
                else if (value == 127.0f) {
                    value_str = _node["map"][total-1].as<std::string>();
                } 
                else {
                    size_t index = (value / 127.0f) * _node["map"].size();
                    value_str = _node["map"][index].as<std::string>();
                } 
            }
            else if ( _node["map"].IsScalar() ) {
                value_str = _node["map"].as<std::string>();
            }
        }
            
        _node["value"] = value_str;
        return updateNode(_node, _term, _status, _channel, _key);
    }
    
    // SCALAR
    else if ( type == TYPE_NUMBER ) {
        float value = _value;

        if ( _node["map"] ) {
            value /= 127.0f;
            if ( _node["map"].IsSequence() ) {
                if ( _node["map"].size() > 1 ) {
                    float total = _node["map"].size() - 1;

                    size_t i_low = value * total;
                    size_t i_high = M_MIN(i_low + 1, size_t(total));
                    float pct = (value * total) - (float)i_low;
                    value = lerp(   _node["map"][i_low].as<float>(),
                                    _node["map"][i_high].as<float>(),
                                    pct );
                }
            }
        }
            
        _node["value"] = value;
        return updateNode(_node, _term, _status, _channel, _key);
    }
    
    // VECTOR
    else if ( type == TYPE_VECTOR ) {
        float pct = _value / 127.0f;
        Vector value = Vector(0.0, 0.0, 0.0);

        if ( _node["map"] ) {
            if ( _node["map"].IsSequence() ) {
                if ( _node["map"].size() > 1 ) {
                    float total = _node["map"].size() - 1;

                    size_t i_low = pct * total;
                    size_t i_high = M_MIN(i_low + 1, size_t(total));

                    value = lerp(   _node["map"][i_low].as<Vector>(),
                                    _node["map"][i_high].as<Vector>(),
                                    (pct * total) - (float)i_low );
                }
            }
        }
            
        _node["value"] = value;
        return updateNode(_node, _term, _status, _channel, _key);
    }

    // COLOR
    else if ( type == TYPE_COLOR ) {
        float pct = _value / 127.0f;
        Color value = Color(0.0, 0.0, 0.0);

        if ( _node["map"] ) {
            if ( _node["map"].IsSequence() ) {
                if ( _node["map"].size() > 1 ) {
                    float total = _node["map"].size() - 1;

                    size_t i_low = pct * total;
                    size_t i_high = M_MIN(i_low + 1, size_t(total));

                    value = lerp(   _node["map"][i_low].as<Color>(),
                                    _node["map"][i_high].as<Color>(),
                                    (pct * total) - (float)i_low );
                }
            }
        }
        
        _node["value"] = value;
        return updateNode(_node, _term, _status, _channel, _key);
    }

    else if (   type == TYPE_UNKNOWN ||
                type == TYPE_MIDI_NOTE || 
                type == TYPE_MIDI_CONTROLLER_CHANGE ||
                type == TYPE_MIDI_TIMING_TICK ) {

        _node["value"] = int(_value);
        return updateNode(_node, _term, _status, _channel, _key);
    }
    // else {
    //     _node["value"] = int(_value);
    //     return updateNode(_node, _term, _status, _channel, _key);
    // }

    return false;
}

AddressList Context::getTargetsForNode(YAML::Node _node) {
    if (_node["out"].IsDefined() ) {
        AddressList keyTargets;
        if (_node["out"].IsSequence()) 
            for (size_t i = 0; i < _node["out"].size(); i++) {
                keyTargets.push_back( parseAddress( _node["out"][i].as<std::string>() ) );
            }
        else if (_node["out"].IsScalar()) {
            Address target = parseAddress( _node["out"].as<std::string>() );
            keyTargets.push_back( target );
        }
        return keyTargets;
    }
    else
        return targets;
}


bool Context::updateNode(YAML::Node _node, 
                        const std::string& _term, unsigned char _status, size_t _channel, 
                        size_t _key) {

    if ( !_node["value"].IsDefined() )
        return false;

    // Define out targets
    AddressList keyTargets = getTargetsForNode(_node);
    DataType type = getKeyDataType(_node);

    YAML::Node value = _node["value"];
    float value_raw = 0;
    if ( _node["value_raw"].IsDefined() )
        value_raw = _node["value_raw"].as<float>();

    // KEY
    std::string name = "unknown";
    if ( _node.IsDefined() ) {
        if ( _node["name"].IsDefined() ) {
            name = _node["name"].as<std::string>();
        }
        else {
            name = ""; 
            if ( _node["channel"].IsDefined() )
                name += toString(_channel) + ",";

            if (_key != 0)
                name += toString(_key);
        } 
    }

    // BUTTON and TOGGLE
    bool result = false;
    if ( type == TYPE_TOGGLE || type == TYPE_BUTTON ) {
        std::string value_str = (value.as<bool>()) ? "on" : "off";
        
        if (_node["map"]) {

            if (_node["map"][value_str]) {

                // If the end mapped string is a sequence
                if (_node["map"][value_str].IsSequence()) {
                    for (size_t i = 0; i < _node["map"][value_str].size(); i++) {
                        std::string prop = name;
                        std::string msg = "";

                        if ( parseString(_node["map"][value_str][i], prop, msg) ) {
                            for (size_t t = 0; t < keyTargets.size(); t++)
                                broadcast(keyTargets[t], prop, msg);
                        }
                    }
                }
                else {
                    std::string prop = name;
                    std::string msg = "";

                    if ( parseString(_node["map"][value_str], prop, msg) ) {
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

        if ( sourcesTerm[_term]->getType() == MIDI_DEVICE ) 
            feedback(_term, _status, _channel, _key, _node["value"].as<bool>() ? 127 : 0);

        result = true;
    }

    // STATE
    else if ( type == TYPE_STRING ) {
        for (size_t t = 0; t < keyTargets.size(); t++)
            broadcast(keyTargets[t], name, value.as<std::string>());

        result = true;
    }

    // SCALAR
    else if ( type == TYPE_NUMBER ) {
        for (size_t t = 0; t < keyTargets.size(); t++)
            broadcast(keyTargets[t], name, value.as<float>());

        result = true;
    }

    // VECTOR
    else if ( type == TYPE_VECTOR ) {
        for (size_t t = 0; t < keyTargets.size(); t++)
            broadcast(keyTargets[t], name, value.as<Vector>());

        result = true;
    }

    // COLOR
    else if ( type == TYPE_COLOR ) {
        for (size_t t = 0; t < keyTargets.size(); t++)
            broadcast(keyTargets[t], name, value.as<Color>());
        
        result = true;
    }

    for (size_t i = 0; i < keyTargets.size(); i++) {
        std::string plug = keyTargets[i].address;

        if (keyTargets[i].protocol == MIDI_PROTOCOL && keyTargets[i].isFile) {
            MidiFile *p;

            if (keyTargets[i].term == nullptr) {
                std::map<std::string, Term*>::iterator it = targetsTerm.find( plug );
                if (it == targetsTerm.end()) {
                    p = new MidiFile(this, plug );
                    targetsTerm[ plug ] = (Term*)p;
                    p = (MidiFile*)it->second;
                }
                else
                    p = (MidiFile*)it->second;
            }
            else 
                p = (MidiFile*)keyTargets[i].term;

            p->trigger(_term, _status, _channel, _key, value_raw);
        }

        else if (keyTargets[i].protocol == MIDI_PROTOCOL && !keyTargets[i].isFile) {
            MidiDevice* p = nullptr;

            if (keyTargets[i].term == nullptr) {
                std::map<std::string, Term*>::iterator it = targetsTerm.find( plug );
                if ( it != targetsTerm.end() )
                    MidiDevice* p = (MidiDevice*)it->second;
            }
            else 
                p = (MidiDevice*)keyTargets[i].term;

            if (p != nullptr) {

                if ( type == TYPE_MIDI_NOTE) {
                    if (value_raw == 0.0)
                        p->trigger( Midi::NOTE_OFF, 0, _key, value_raw);
                    else 
                        p->trigger( Midi::NOTE_ON, 0, _key, value_raw );
                }

                else if ( type == TYPE_MIDI_CONTROLLER_CHANGE )
                    p->trigger( Midi::CONTROLLER_CHANGE, 0, _key, value_raw );
                
                else if ( type == TYPE_MIDI_TIMING_TICK )
                    p->trigger( Midi::TIMING_TICK, 0 );

                else if ( type == TYPE_UNKNOWN )
                    p->trigger( _status, _channel, _key, value_raw);
                
            }
        }

        else {
            std::string path = name + "/" + toString(type);

            if (type == TYPE_UNKNOWN || type == TYPE_MIDI_NOTE || type == TYPE_MIDI_TIMING_TICK || type == TYPE_MIDI_TIMING_TICK) {
                if (_status == Midi::NOTE_ON)
                    path = name + "/" + std::string( (value_raw == 0)? "NOTE_OFF" : "NOTE_ON" );
                else
                    path = name + "/" + Midi::statusByteToName(_status);
            }

            broadcast(keyTargets[i], path, Vector(_channel, _key, value_raw));
        }
    }

    return result;
}

bool Context::feedback(const std::string& _term, unsigned char _status, size_t _channel, size_t _key, size_t _value) {
    std::map<std::string, Term*>::iterator it = sourcesTerm.find(_term);
    if (it != sourcesTerm.end()) {
        ((MidiDevice*)it->second)->trigger( _status, _channel, _key, _value);
        return true;
    }

    return false;
}


