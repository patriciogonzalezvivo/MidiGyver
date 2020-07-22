#pragma once

#include "yaml-cpp/yaml.h"

#include "strings.h"
#include "../JSContext.h"

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

inline bool getDouble(const YAML::Node& node, double& result, bool allowTrailingJunk = false) {
    if (node.IsScalar()) {
        const std::string& scalar = node.Scalar();
        result = toDouble(scalar);
        return true;
        // int size = static_cast<int>(scalar.size());
        // int count = 0;
        // double value = ff::stod(scalar.data(), size, &count);
        // if (count > 0 && (count == size || allowTrailingJunk)) {
        //     result = value;
        //     return true;
        // }
    }
    return false;
}

inline bool getFloat(const YAML::Node& node, float& result, bool allowTrailingJunk = false) {
    if (node.IsScalar()) {
        const std::string& scalar = node.Scalar();
        result = toFloat(scalar);
        return true;
        // int size = static_cast<int>(scalar.size());
        // int count = 0;
        // float value = ff::stof(scalar.data(), size, &count);

        // if (count > 0 && (count == size || allowTrailingJunk)) {
        //     result = value;
        //     return true;
        // }
    }
    return false;
}

inline bool getInt(const YAML::Node& node, int& result, bool allowTrailingJunk = false) {
    double value;
    if (getDouble(node, value, allowTrailingJunk)) {
        result = static_cast<int>(std::round(value));
        return true;
    }
    return false;
}

inline bool getBool(const YAML::Node& node, bool& result) {
    return YAML::convert<bool>::decode(node, result);
}

// inline double getDoubleOrDefault(const YAML::Node& node, double defaultValue, bool allowTrailingJunk) {
//     getDouble(node, defaultValue, allowTrailingJunk);
//     return defaultValue;
// }

// inline float getFloatOrDefault(const YAML::Node& node, float defaultValue, bool allowTrailingJunk) {
//     getFloat(node, defaultValue, allowTrailingJunk);
//     return defaultValue;
// }


// inline int getIntOrDefault(const YAML::Node& node, int defaultValue, bool allowTrailingJunk = false) {
//     getInt(node, defaultValue, allowTrailingJunk);
//     return defaultValue;
// }

// inline bool getBoolOrDefault(const YAML::Node& node, bool defaultValue) {
//     getBool(node, defaultValue);
//     return defaultValue;
// }


// Convert a scalar node to a boolean, double, or string (in that order)
// and for the first conversion that works, push it to the top of the JS stack.
inline JSValue pushYamlScalarAsJsPrimitive(JSContext& _js, const YAML::Node& _node) {
    bool booleanValue = false;
    float numberValue = 0.;
    if (getBool(_node, booleanValue))
        return _js.newBoolean(booleanValue);
    // else if (getDouble(_node, numberValue))
    //     return _js.newNumber(numberValue);
    else if (getFloat(_node, numberValue))
        return _js.newNumber(numberValue);
    else
        return _js.newString(_node.Scalar());
}

inline JSValue pushYamlScalarAsJsFunctionOrString(JSContext& _js, const YAML::Node& _node) {
    auto value = _js.newFunction(_node.Scalar());
    if (value) {
        return value;
    }
    return _js.newString(_node.Scalar());
}

inline JSValue parseSceneGlobals(JSContext& _js, const YAML::Node& _node) {
    switch(_node.Type()) {
    case YAML::NodeType::Scalar: {
        auto& scalar = _node.Scalar();
        if (scalar.compare(0, 8, "function") == 0) {
            return pushYamlScalarAsJsFunctionOrString(_js, _node);
        }
        return pushYamlScalarAsJsPrimitive(_js, _node);
    }
    case YAML::NodeType::Sequence: {
        auto jsArray = _js.newArray();
        for (size_t i = 0; i < _node.size(); i++) {
            jsArray.setValueAtIndex(i, parseSceneGlobals(_js, _node[i]));
        }
        return jsArray;
    }
    case YAML::NodeType::Map: {
        auto jsObject = _js.newObject();
        for (const auto& entry : _node) {
            if (!entry.first.IsScalar()) {
                continue; // Can't put non-scalar keys in JS objects.
            }
            jsObject.setValueForProperty(entry.first.Scalar(), parseSceneGlobals(_js, entry.second));
        }
        return jsObject;
    }
    default:
        return _js.newNull();
    }
}

