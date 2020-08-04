#include "Context.h"

#include "ops/osc.h"
#include "ops/nodes.h"
#include "ops/strings.h"

#include "types/Vector.h"
#include "types/Color.h"

Context::Context() {
}

Context::~Context() {
}

bool Context::load(const std::string& _filename) {
    config = YAML::LoadFile(_filename);

    // JS Globals
    JSValue global = parseNode(js, config["global"]);
    js.setGlobalValue("global", std::move(global));

    // JS Functions
    uint32_t id = 0;

    if (config["pulse"].IsSequence()) {
        for (size_t i = 0; i < config["pulse"].size(); i++) {
            YAML::Node n = config["pulse"][i];
            std::string pulseName = n["name"].as<std::string>();

            // std::cout << "Adding pulse: " << pulseName << std::endl;
            devicesData[pulseName].keyMap[i] = i;

            if (n["shape"].IsDefined()) {
                std::string function = n["shape"].as<std::string>();
                if ( js.setFunction(id, function) ) {
                    shapeFncs[pulseName + "_0"] = id;
                    id++;
                }
            }

            Pulse* p = new Pulse(this, i);
            if (n["interval"].IsDefined()) 
                p->start(n["interval"].as<int>());
            // // std::string pulseName = n["name"].as<std::string>();
            // size_t index = i;

            // float counter = 0;
            // p->setInterval([&]() {
            //     // if (counter >= 1) {
            //         configMutex.lock();
            //         // Context *ctx = static_cast<Context*>(this);
            //         std::cout << " + " << pulseName << " : " << counter << std::endl;
            //         // if (ctx->shapeKeyValue(ctx->config["pulse"][index], pulseName, 0, &counter)) {
            //             std::cout << " > " << counter << std::endl;
            //         //     ctx->mapKeyValue(ctx->config["pulse"][index], pulseName, 0, counter);
            //         // }
            //         configMutex.unlock();
            //     // }



            // }, interval);

            pulses.push_back(p);
        }
    }

    if (config["in"].IsMap()) {
        for (YAML::const_iterator dev = config["in"].begin(); dev != config["in"].end(); ++dev) {
            std::string device = dev->first.as<std::string>();

            if (config["in"][device].IsMap()) {
                for (YAML::const_iterator it = config["in"][device].begin(); it != config["in"][device].end(); ++it) {
                    std::string key = it->first.as<std::string>();
                    if (config["in"][device][key]["shape"].IsDefined()) {
                        std::string function = config["in"][device][key]["shape"].as<std::string>();
                        if ( js.setFunction(id, function) ) {
                            shapeFncs[device + "_" + key] = id;
                            id++;
                        }
                    }
                }
            }

            else if (config["in"][device].IsSequence()) {
                for (size_t i = 0; i < config["in"][device].size(); i++) {
                    size_t key = i;

                    if (config["in"][device][i]["key"].IsDefined())
                        key = config["in"][device][i]["key"].as<size_t>();

                    devicesData[device].keyMap[key] = i;

                    if (config["in"][device][i]["shape"].IsDefined()) {
                        std::string function = config["in"][device][i]["shape"].as<std::string>();
                        if ( js.setFunction(id, function) ) {
                            shapeFncs[device + "_" + toString(key)] = id;
                            id++;
                        }
                    }
                }
            }
        }
    }

    // Define OSC out targets
    if (config["out"].IsSequence())
        for (size_t i = 0; i < config["out"].size(); i++)
            targets.push_back(config["out"][i].as<std::string>());
    else if (config["out"].IsScalar())
        targets.push_back(config["out"].as<std::string>());

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

bool Context::updateDevice(const std::string& _device) {
    if ( config["in"][_device].IsMap() ) {
        for (YAML::const_iterator it = config["in"][_device].begin(); it != config["in"][_device].end(); ++it) {
            std::string key = it->first.as<std::string>();
            if (it->second["value"])
                updateKey(it->second, _device, toInt(key));
        }
    }

    else if (config["in"][_device].IsSequence()) {
        for (size_t i = 0; i < config["in"][_device].size(); i++) {
            size_t key = i;
            if (config["in"][_device][i]["key"].IsDefined())
                key = config["in"][_device][i]["key"].as<size_t>();
            updateKey(config["in"][_device][i], _device, key);
        }
    }

    return true;
}

bool Context::doKeyExist(const std::string& _device, size_t _key) {
     if ( !config["in"][_device].IsNull() ) {

        if (config["in"][_device].IsMap()) {
            std::string key = toString(_key);
            if (config["in"][_device][key].IsDefined())
                return true;
        }

        else if (config["in"][_device].IsSequence()) {
            return devicesData[_device].keyMap.find(_key) != devicesData[_device].keyMap.end(); 
        }
    }
    return false;
}

YAML::Node  Context::getKeyNode(const std::string& _device, size_t _key) {
    if (config["in"][_device].IsMap()) {
        std::string key = toString(_key);
        return config["in"][_device][key];
    }

    else if (config["in"][_device].IsSequence()) {
        size_t key = devicesData[_device].keyMap[_key];
        return config["in"][_device][key];
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
        }
    }
    return TYPE_UNKNOWN;
}

std::string Context::getKeyName(YAML::Node _keynode) {
    if ( _keynode.IsDefined() ) {
        if ( _keynode["name"].IsDefined() ) {
            return _keynode["name"].as<std::string>();
        }
    }
    return "unknownName";
}

bool Context::shapeKeyValue(YAML::Node _keynode, const std::string& _device, const std::string& _type, size_t _key, float* _value) {
    if ( _keynode["shape"].IsDefined() ) {

        js.setGlobalValue("device", js.newString(_device));
        js.setGlobalValue("type", js.newString(_type));
        js.setGlobalValue("key", js.newNumber(_key));
        js.setGlobalValue("value", js.newNumber(*_value));

        JSValue keyData = parseNode(js, _keynode);
        js.setGlobalValue("data", std::move(keyData));
        
        std::string key = toString(_key);
        JSValue result = js.getFunctionResult( shapeFncs[_device + "_" + key] );
        if (!result.isNull()) {

            if (result.isString()) {
                std::cout << "Update result on string: " << result.toString() << " but don't know what to do with it"<< std::endl;
                return false;
            }

            else if (result.isObject()) {
                for (size_t j = 0; j < devices.size(); j++) {
                    JSValue d = result.getValueForProperty(devices[j]);
                    if (!d.isUndefined()) {
                        for (size_t i = 0; i < d.getLength(); i++) {
                            JSValue el = d.getValueAtIndex(i);
                            if (el.isArray()) {
                                if (el.getLength() == 2) {
                                    size_t k = el.getValueAtIndex(0).toInt();
                                    float v = el.getValueAtIndex(1).toFloat();
                                    // std::cout << "trigger(" << devices[j] << "," << k << "," << v << ")" << std::endl;
                                    mapKeyValue(_keynode, devices[j], k, v);
                                }
                            }
                        }
                    } 
                }
                return false;
            }

            else if (result.isArray()) {
                for (size_t i = 0; i < result.getLength(); i++) {
                    JSValue el = result.getValueAtIndex(i);
                    if (el.isArray()) {
                        if (el.getLength() == 2) {
                            size_t k = el.getValueAtIndex(0).toInt();
                            float v = el.getValueAtIndex(1).toFloat();
                            // std::cout << "trigger (" << _device << "," << k << "," << v << ")" << std::endl;
                            mapKeyValue(_keynode, _device, k, v);
                        }
                    }
                }
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

bool Context::mapKeyValue(YAML::Node _keynode, const std::string& _device, size_t _key, float _value) {

    std::string name = getKeyName(_keynode);
    DataType type = getKeyDataType(_keynode);
    _keynode["value_raw"] = _value;

    // BUTTON
    if (type == TYPE_BUTTON) {
        bool value = _value > 0;
        _keynode["value"] = value;
        return updateKey(_keynode, _device, _key);
    }
    
    // TOGGLE
    else if ( type == TYPE_TOGGLE ) {
        if (_value > 0) {
            bool value = false;
                
            if (_keynode["value"])
                value = _keynode["value"].as<bool>();

            _keynode["value"] = !value;
            return updateKey(_keynode, _device, _key);
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
        return updateKey(_keynode, _device, _key);
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
                    size_t i_high = std::min(i_low + 1, size_t(total));
                    float pct = (value * total) - (float)i_low;
                    value = lerp(   _keynode["map"][i_low].as<float>(),
                                    _keynode["map"][i_high].as<float>(),
                                    pct );
                }
            }
        }
            
        _keynode["value"] = value;
        return updateKey(_keynode, _device, _key);
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
                    size_t i_high = std::min(i_low + 1, size_t(total));

                    value = lerp(   _keynode["map"][i_low].as<Vector>(),
                                    _keynode["map"][i_high].as<Vector>(),
                                    (pct * total) - (float)i_low );
                }
            }
        }
            
        _keynode["value"] = value;
        return updateKey(_keynode, _device, _key);
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
                    size_t i_high = std::min(i_low + 1, size_t(total));

                    value = lerp(   _keynode["map"][i_low].as<Color>(),
                                    _keynode["map"][i_high].as<Color>(),
                                    (pct * total) - (float)i_low );
                }
            }
        }
        
        _keynode["value"] = value;
        return updateKey(_keynode, _device, _key);
    }

    return false;
}

bool Context::updateKey(YAML::Node _keynode, const std::string& _device, size_t _key) {

    if ( _keynode["value"].IsDefined() ) {
        DataType type = getKeyDataType(_keynode);
        // BUTTONs and TOGGLEs need to change state on the device
        if (type == TYPE_BUTTON || type == TYPE_TOGGLE) {
            bool value = _keynode["value"].as<bool>();

            std::vector<unsigned char> msg;
            msg.push_back( 0xB0 );
            msg.push_back( _key );
            msg.push_back( value ? 127 : 0 );
            devicesData[_device].out->sendMessage( &msg );   
        }
        return sendKeyValue(_keynode);
    }

    return false;
}

bool Context::sendKeyValue(YAML::Node _keynode) {
    if ( !_keynode["value"].IsDefined() )
        return false;

    // Define OSC out targets
    std::vector<std::string> keyTargets;
    if (_keynode["out"].IsDefined() ) {
        if (_keynode["out"].IsSequence()) 
            for (size_t i = 0; i < _keynode["out"].size(); i++)
                keyTargets.push_back(_keynode["out"][i].as<std::string>());
        else if (_keynode["out"].IsScalar())
            keyTargets.push_back(_keynode["out"].as<std::string>());
    }
    else
        keyTargets = targets;
        
    // KEY
    std::string name = getKeyName(_keynode);
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
                            for (size_t t = 0; t < keyTargets.size(); t++) {
                                if (keyTargets[t].rfind("osc://", 0) == 0)
                                    sendValue(parseOscTarget(keyTargets[t]), prop, msg );
                                else if (keyTargets[t] == "csv")
                                    std::cout << prop << "," << msg << std::endl;
                            }
                        }

                    }
                }
                else {
                    std::string prop = name;
                    std::string msg = "";

                    if ( parseString(_keynode["map"][value_str], prop, msg) ) {
                        for (size_t t = 0; t < keyTargets.size(); t++) {
                            if (keyTargets[t].rfind("osc://", 0) == 0)
                                sendValue(parseOscTarget(keyTargets[t]), prop, msg );
                            else if (keyTargets[t] == "csv")
                                std::cout << prop << "," << msg << std::endl;
                        }
                    }
                }

            }

        }
        else {
            for (size_t t = 0; t < keyTargets.size(); t++) {
                if (keyTargets[t].rfind("osc://", 0) == 0)
                    sendValue(parseOscTarget(keyTargets[t]), name, value_str);
                else if (keyTargets[t] == "csv")
                    std::cout << name << "," << (value_str) << std::endl;
            }

            return true;
        }
    }

    // STATE
    else if ( type == TYPE_STRING ) {
        for (size_t t = 0; t < keyTargets.size(); t++) {
            if (keyTargets[t].rfind("osc://", 0) == 0)
                sendValue(parseOscTarget(keyTargets[t]), name, value.as<std::string>());
            else if (keyTargets[t] == "csv")
                std::cout << name << "," << value.as<std::string>() << std::endl;
        }

        return true;
    }

    // SCALAR
    else if ( type == TYPE_NUMBER ) {
        for (size_t t = 0; t < keyTargets.size(); t++) {
            if (keyTargets[t].rfind("osc://", 0) == 0)
                sendValue(parseOscTarget(keyTargets[t]), name, value.as<float>());
            else if (keyTargets[t] == "csv")
                std::cout << name << "," << value.as<float>() << std::endl;
        }
        return true;
    }

    // VECTOR
    else if ( type == TYPE_VECTOR ) {
        for (size_t t = 0; t < keyTargets.size(); t++) {
            if (keyTargets[t].rfind("osc://", 0) == 0)
                sendValue(parseOscTarget(keyTargets[t]), name, value.as<Vector>());
            else if (keyTargets[t] == "csv")
                std::cout << name << "," << value.as<Vector>() << std::endl;
        }

        return true;
    }

    // COLOR
    else if ( type == TYPE_COLOR ) {
        for (size_t t = 0; t < keyTargets.size(); t++) {
            if (keyTargets[t].rfind("osc://", 0) == 0)
                sendValue(parseOscTarget(keyTargets[t]), name, value.as<Color>());
            else if (keyTargets[t] == "csv")
                std::cout << name << "," << value.as<Color>() << std::endl;
        }
        return true;
    }

    return false;
}

