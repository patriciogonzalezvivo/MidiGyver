#pragma once

#include "yaml-cpp/yaml.h"
#include "strings.h"

inline bool parseString(const YAML::Node& _node, std::string& _propName, std::string& _msg ) {
    if (_node) {
        std::string value = _node.as<std::string>();
        stringReplace(value, ',');
        std::vector<std::string> el = split(value, ',', true);

        if (el.size() == 1) {
            _msg = value;
            return true;
        }
        else {
            _propName = el[0];
            _msg = el[1];
            return true;
        }
    }

    return false;
}

inline std::string getMatchingKey(const YAML::Node& _node, const std::string& _target) {
    for (YAML::const_iterator it = _node.begin(); it != _node.end(); ++it) {
        std::string key = it->first.as<std::string>();
        if (match(key.c_str(), _target.c_str()))
            return key;
    }

    return "";
}