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
    bool osc = false;
    std::string oscAddress = "localhost";
    std::string oscPort = "8000";
    std::string oscFolder = "/";
    if (config["out"]["osc"]) {
        osc = true;
        if (config["out"]["osc"]["address"]) 
            oscAddress = config["out"]["osc"]["address"].as<std::string>();
        if (config["out"]["osc"]["port"]) 
            oscPort = config["out"]["osc"]["port"].as<std::string>();
        if (config["out"]["osc"]["folder"])
            oscFolder = config["out"]["osc"]["folder"].as<std::string>();
    }

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
                                sendValue(oscAddress, oscPort, oscFolder + prop, msg );
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
                            sendValue(oscAddress, oscPort, oscFolder + prop, msg );
                        if (csv)
                            std::cout << name << "," << msg << std::endl;
                    }
                }

            }

        }
        else {

            if (osc)
                sendValue(oscAddress, oscPort, oscFolder + name, value_str);

            if (csv)
                std::cout << name << "," << (value_str) << std::endl;

            return true;
        }
    }

    else if ( type == "scalar" ) {
        if (osc)
            sendValue(oscAddress, oscPort, oscFolder + name, value.as<float>());

        if (csv)
            std::cout << name << "," << value.as<float>() << std::endl;

        return true;
    }

    else if ( type == "states" ) {
        if (osc)
            sendValue(oscAddress, oscPort, oscFolder + name, value.as<std::string>());

        if (csv)
            std::cout << name << "," << value.as<std::string>() << std::endl;

        return true;
    }

    else if ( type == "vector" ) {
        if (osc)
            sendValue(oscAddress, oscPort, oscFolder + name, value.as<Vector>());

        if (csv)
            std::cout << name << "," << value.as<Vector>() << std::endl;

        return true;
    }

    else if ( type == "color" ) {
        if (osc)
            sendValue(oscAddress, oscPort, oscFolder + name, value.as<Color>());

        if (csv)
            std::cout << name << "," << value.as<Color>() << std::endl;

        return true;
    }

    return false;
}

