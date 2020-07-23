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

    auto jsValue = parseNode(js, config["global"]);
    js.setGlobalValue("global", std::move(jsValue));

    // Define OSC out targets
    if (config["out"]["osc"].IsDefined()) {
        if (config["out"]["osc"].IsMap()) {
            oscTargets["default"] = parseOscTarget(config["out"]["osc"]);
        }
        else if (config["out"]["osc"].IsSequence()) {
            for (size_t i = 0; i < config["out"]["osc"].size(); i++) {
                if (i == 0 ) {
                    oscTargets["default"] = parseOscTarget(config["out"]["osc"][i]);
                }
                else if (config["out"]["osc"][i]["name"].IsDefined()) {
                    std::string key = config["out"]["osc"][i]["name"].as<std::string>(); 
                    oscTargets[key] = parseOscTarget(config["out"]["osc"][i]);
                }
            }
        }
    }

    // uint32_t id = 0;
    // js.setFunction(id, "function() { return global.time }");
    // auto result = js.getFunctionResult(id);

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
                updateKey(_device, key);
        }
    }
    return true;
}

bool Context::updateKey(const std::string& _device, const std::string& _key) {
    
    // CSV
    bool csv = false;
    if (config["out"]["csv"])
        csv = true;

    // OSC
    bool osc = oscTargets.size() > 0;
    std::vector<std::string> targets;
    if (config["in"][_device][_key]["osc"].IsDefined() ) {
        if (config["in"][_device][_key]["osc"].IsScalar())
            targets.push_back( config["in"][_device][_key]["osc"].as<std::string>() );

        else if (config["in"][_device][_key]["osc"].IsSequence())
            for (size_t i = 0; config["in"][_device][_key]["osc"].size(); i++)
                targets.push_back( config["in"][_device][_key]["osc"][i].as<std::string>() );
    }
    else 
        targets.push_back("default");
        
    // KEY
    std::string name = _key;
    std::string type = "unknown";
    YAML::Node  value = config["in"][_device][_key]["value"];

    if ( config["in"][_device][_key]["value"].IsNull() )
        return false;

    if ( config["in"][_device][_key]["name"] )
        name = config["in"][_device][_key]["name"].as<std::string>();

    if ( config["in"][_device][_key]["type"] )
        type =  config["in"][_device][_key]["type"].as<std::string>();

    if ( type == "toggle" || type == "button" ) {
        std::string value_str = (value.as<bool>()) ? "on" : "off";
        
        if (config["in"][_device][_key]["map"]) {

            if (config["in"][_device][_key]["map"][value_str]) {

                // If the end mapped string is a sequence
                if (config["in"][_device][_key]["map"][value_str].IsSequence()) {
                    for (size_t i = 0; i < config["in"][_device][_key]["map"][value_str].size(); i++) {
                        std::string prop = name;
                        std::string msg = "";

                        if ( parseString(config["in"][_device][_key]["map"][value_str][i], prop, msg) ) {
                            if (osc) 
                                for (size_t t = 0; t < targets.size(); t++) 
                                    sendValue(oscTargets[targets[t]], prop, msg );
                            if (csv)
                                std::cout << prop << "," << msg << std::endl;
                        }

                    }
                }
                else {
                    std::string prop = name;
                    std::string msg = "";

                    if ( parseString(config["in"][_device][_key]["map"][value_str], prop, msg) ) {
                        if (osc)
                            for (size_t t = 0; t < targets.size(); t++) 
                                    sendValue(oscTargets[targets[t]], prop, msg );
                        if (csv)
                            std::cout << name << "," << msg << std::endl;
                    }
                }

            }

        }
        else {

            if (osc)
                for (size_t t = 0; t < targets.size(); t++) 
                    sendValue(oscTargets[targets[t]], name, value_str);

            if (csv)
                std::cout << name << "," << (value_str) << std::endl;

            return true;
        }
    }

    else if ( type == "scalar" ) {
        if (osc)
            for (size_t t = 0; t < targets.size(); t++) 
                sendValue(oscTargets[targets[t]], name, value.as<float>());

        if (csv)
            std::cout << name << "," << value.as<float>() << std::endl;

        return true;
    }

    else if ( type == "states" ) {
        if (osc)
            for (size_t t = 0; t < targets.size(); t++) 
                sendValue(oscTargets[targets[t]], name, value.as<std::string>());

        if (csv)
            std::cout << name << "," << value.as<std::string>() << std::endl;

        return true;
    }

    else if ( type == "vector" ) {
        if (osc)
            for (size_t t = 0; t < targets.size(); t++) 
                sendValue(oscTargets[targets[t]], name, value.as<Vector>());

        if (csv)
            std::cout << name << "," << value.as<Vector>() << std::endl;

        return true;
    }

    else if ( type == "color" ) {
        if (osc)
            for (size_t t = 0; t < targets.size(); t++) 
                sendValue(oscTargets[targets[t]], name, value.as<Color>());

        if (csv)
            std::cout << name << "," << value.as<Color>() << std::endl;

        return true;
    }

    return false;
}

