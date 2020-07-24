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
    if (config["in"].IsMap()) {
        for (YAML::const_iterator dev = config["in"].begin(); dev != config["in"].end(); ++dev) {
            std::string dev_key = dev->first.as<std::string>();
            for (YAML::const_iterator it = config["in"][dev_key].begin(); it != config["in"][dev_key].end(); ++it) {
                std::string key = it->first.as<std::string>();
                if (config["in"][dev_key][key]["update"].IsDefined()) {
                    std::string function = config["in"][dev_key][key]["update"].as<std::string>();
                    if ( js.setFunction(id, function) ) {
                        jsFunctions[dev_key + "_" + key] = id;
                        id++;
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
                updateKey(_device, toInt(key));
        }
    }
    return true;
}

bool Context::doKeyExist(const std::string& _device, size_t _key) {
     if ( !config["in"][_device].IsNull() ) {
        std::string key = toString(_key);
        if (config["in"][_device][key].IsDefined())
            return true;
    }
    return false;
}

YAML::Node  Context::getKeyNode(const std::string& _device, size_t _key) {
    std::string key = toString(_key);
    return config["in"][_device][key];
}

DataType Context::getKeyDataType(const std::string& _device, size_t _key) {
    YAML::Node node = getKeyNode(_device, _key);
    if ( node.IsDefined() ) {
        if ( node["type"].IsDefined() ) {
            std::string typeString =  node["type"].as<std::string>();
            if (typeString == "button")
                return button_type;
            else if (typeString == "toggle")
                return toggle_type;
            else if (typeString == "state")
                return state_type;
            else if (typeString == "scalar")
                return scalar_type;
            else if (typeString == "vector")
                return vector_type;
            else if (typeString == "color")
                return color_type;
        }
    }
    return unknown_type;
}

std::string Context::getKeyName(const std::string& _device, size_t _key) {
    YAML::Node node = getKeyNode(_device, _key);
    if ( node.IsDefined() ) {
        if ( node["name"].IsDefined() ) {
            return node["name"].as<std::string>();
        }
    }
    return toString(_key);
}

bool Context::mapKeyValue(const std::string& _device, size_t _key, size_t _value) {
    if (!doKeyExist(_device, _key))
        return false;

    std::string key = toString(_key);
    std::string name = getKeyName(_device, _key);
    DataType type = getKeyDataType(_device, _key);

    // BUTTON
    if (type == button_type) {
        bool value = (int)_value == 127;
        config["in"][_device][key]["value"] = value;
        return updateKey(_device, _key);
    }
    
    // TOGGLE
    else if ( type == toggle_type ) {
        if ((int)_value == 127) {
            bool value = false;
                
            if (config["in"][_device][key]["value"])
                value = config["in"][_device][key]["value"].as<bool>();

            config["in"][_device][key]["value"] = !value;
            return updateKey(_device, _key);
        }
    }

    // STATE
    else if ( type == state_type ) {
        int value = (int)_value;
        std::string value_str = toString(value);

        if ( config["in"][_device][key]["map"] ) {
            if ( config["in"][_device][key]["map"].IsSequence() ) {
                float total = config["in"][_device][key]["map"].size();

                if (value == 127.0f) {
                    value_str = config["in"][_device][key]["map"][total-1].as<std::string>();
                } 
                else {
                    size_t index = (value / 127.0f) * config["in"][_device][key]["map"].size();
                    value_str = config["in"][_device][key]["map"][index].as<std::string>();
                } 
            }
        }
            
        config["in"][_device][key]["value"] = value_str;
        return updateKey(_device, _key);
    }
    
    // SCALAR
    else if ( type == scalar_type ) {
        float value = (float)_value / 127.0f;

        if ( config["in"][_device][key]["map"] ) {
            if ( config["in"][_device][key]["map"].IsSequence() ) {
                if ( config["in"][_device][key]["map"].size() > 1 ) {
                    float total = config["in"][_device][key]["map"].size() - 1;

                    size_t i_low = value * total;
                    size_t i_high = std::min(i_low + 1, size_t(total));
                    float pct = (value * total) - (float)i_low;
                    value = lerp(   config["in"][_device][key]["map"][i_low].as<float>(),
                                    config["in"][_device][key]["map"][i_high].as<float>(),
                                    pct );
                }
            }
        }
            
        config["in"][_device][key]["value"] = value;
        return updateKey(_device, _key);
    }
    
    // VECTOR
    else if ( type == vector_type ) {
        float pct = (float)_value / 127.0f;
        Vector value = Vector(0.0, 0.0, 0.0);

        if ( config["in"][_device][key]["map"] ) {
            if ( config["in"][_device][key]["map"].IsSequence() ) {
                if ( config["in"][_device][key]["map"].size() > 1 ) {
                    float total = config["in"][_device][key]["map"].size() - 1;

                    size_t i_low = pct * total;
                    size_t i_high = std::min(i_low + 1, size_t(total));

                    value = lerp(   config["in"][_device][key]["map"][i_low].as<Vector>(),
                                    config["in"][_device][key]["map"][i_high].as<Vector>(),
                                    (pct * total) - (float)i_low );
                }
            }
        }
            
        config["in"][_device][key]["value"] = value;
        return updateKey(_device, _key);
    }

    // COLOR
    else if ( type == color_type ) {
        float pct = (float)_value / 127.0f;
        Color value = Color(0.0, 0.0, 0.0);

        if ( config["in"][_device][key]["map"] ) {
            if ( config["in"][_device][key]["map"].IsSequence() ) {
                if ( config["in"][_device][key]["map"].size() > 1 ) {
                    float total = config["in"][_device][key]["map"].size() - 1;

                    size_t i_low = pct * total;
                    size_t i_high = std::min(i_low + 1, size_t(total));

                    value = lerp(   config["in"][_device][key]["map"][i_low].as<Color>(),
                                    config["in"][_device][key]["map"][i_high].as<Color>(),
                                    (pct * total) - (float)i_low );
                }
            }
        }
        
        config["in"][_device][key]["value"] = value;
        return updateKey(_device, _key);
    }

    return false;
}

bool Context::updateKey(const std::string& _device, size_t _key) {
    DataType type = getKeyDataType(_device, _key);
    std::string key = toString(_key);

    if ( config["in"][_device][key]["update"].IsDefined() ) {

        js.setGlobalValue("device", js.newString(_device));
        js.setGlobalValue("key", js.newNumber(_key));
        YAML::Node keyNode = getKeyNode(_device, _key);
        JSValue keyData = parseNode(js, keyNode);
        js.setGlobalValue("data", std::move(keyData));
        
        JSValue result = js.getFunctionResult( jsFunctions[_device + "_" + key] );

        if (!result.isNull()) {
            if (result.isString())
                std::cout << "Update result on string: " << result.toString() << std::endl;
            else if (result.isNumber())
                std::cout << "Update result on number: " << result.toFloat() << std::endl;
            else if (result.isBoolean())
                std::cout << "Update result on boolean: " << result.toBool() << std::endl;
        }

    }

    if ( config["in"][_device][key]["value"].IsDefined() ) {
        // BUTTONs and TOGGLEs need to change state on the device
        if (type == button_type || type == toggle_type) {
            bool value = config["in"][_device][key]["value"].as<bool>();

            std::vector<unsigned char> msg;
            msg.push_back( 0xB0 );
            msg.push_back( _key );
            msg.push_back( value ? 127 : 0 );
            devicesOut[_device]->sendMessage( &msg );   
        }
        return sendKeyValue(_device, _key);
    }

    return false;
}

bool Context::sendKeyValue(const std::string& _device, size_t _key) {
    std::string key = toString(_key);

    if ( !config["in"][_device][key]["value"].IsDefined() )
        return false;

    // Define OSC out targets
    std::vector<std::string> keyTargets;
    if (config["in"][_device][key]["out"].IsDefined() ) {
        if (config["in"][_device][key]["out"].IsSequence()) 
            for (size_t i = 0; i < config["in"][_device][key]["out"].size(); i++)
                keyTargets.push_back(config["in"][_device][key]["out"][i].as<std::string>());
        else if (config["in"][_device][key]["out"].IsScalar())
            keyTargets.push_back(config["in"][_device][key]["out"].as<std::string>());
    }
    else
        keyTargets = targets;
        
    // KEY
    std::string name = getKeyName(_device, _key);
    DataType type = getKeyDataType(_device, _key);
    YAML::Node value = config["in"][_device][key]["value"];

    // BUTTON and TOGGLE
    if ( type == toggle_type || type == button_type ) {
        std::string value_str = (value.as<bool>()) ? "on" : "off";
        
        if (config["in"][_device][key]["map"]) {

            if (config["in"][_device][key]["map"][value_str]) {

                // If the end mapped string is a sequence
                if (config["in"][_device][key]["map"][value_str].IsSequence()) {
                    for (size_t i = 0; i < config["in"][_device][key]["map"][value_str].size(); i++) {
                        std::string prop = name;
                        std::string msg = "";

                        if ( parseString(config["in"][_device][key]["map"][value_str][i], prop, msg) ) {
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

                    if ( parseString(config["in"][_device][key]["map"][value_str], prop, msg) ) {
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
    else if ( type == state_type ) {
        for (size_t t = 0; t < keyTargets.size(); t++) {
            if (keyTargets[t].rfind("osc://", 0) == 0)
                sendValue(parseOscTarget(keyTargets[t]), name, value.as<std::string>());
            else if (keyTargets[t] == "csv")
                std::cout << name << "," << value.as<std::string>() << std::endl;
        }

        return true;
    }

    // SCALAR
    else if ( type == scalar_type ) {
        for (size_t t = 0; t < keyTargets.size(); t++) {
            if (keyTargets[t].rfind("osc://", 0) == 0)
                sendValue(parseOscTarget(keyTargets[t]), name, value.as<float>());
            else if (keyTargets[t] == "csv")
                std::cout << name << "," << value.as<float>() << std::endl;
        }
        return true;
    }

    // VECTOR
    else if ( type == vector_type ) {
        for (size_t t = 0; t < keyTargets.size(); t++) {
            if (keyTargets[t].rfind("osc://", 0) == 0)
                sendValue(parseOscTarget(keyTargets[t]), name, value.as<Vector>());
            else if (keyTargets[t] == "csv")
                std::cout << name << "," << value.as<Vector>() << std::endl;
        }

        return true;
    }

    // COLOR
    else if ( type == color_type ) {
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

