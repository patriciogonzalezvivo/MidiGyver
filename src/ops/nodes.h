#pragma once

#include "yaml-cpp/yaml.h"

#include "osc.h"
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

inline int getMatchingKey(const std::vector<std::string>& _list, const std::string& _pattern) {
    for (size_t i = 0; i < _list.size(); i++ ) {
        if (match( _pattern.c_str(),_list[i].c_str()))
            return i;
    }

    return -1;
}

inline bool getFloat(const YAML::Node& node, float& result, bool allowTrailingJunk = false) {
    if (node.IsScalar()) {
        const std::string& scalar = node.Scalar();
        float value = toFloat(scalar);

        try {
            if (value == node.as<float>()) {
                result = value;
                return true;
            }
        }
        catch (YAML::BadConversion& e) {
        }
    }
    return false;
}

inline bool getBool(const YAML::Node& node, bool& result) {
    return YAML::convert<bool>::decode(node, result);
}

inline std::vector<size_t> getArrayOfKeys(const YAML::Node& node) {
    std::vector<size_t> result;

    if (node.IsScalar()) {
        std::string key = node.as<std::string>();

        if ( isInt(key) ) {
            result.push_back( node.as<size_t>() );
        }
        else {
            std::vector<size_t> sub = toIntArray(key);
            result.insert(result.end(), sub.begin(), sub.end());
        }
    }
    else if (node.IsSequence()) {
        for (size_t j = 0; j < node.size(); j++) {
            size_t key = node[j].as<size_t>();
            result.push_back( key );
        }
    }

    return result;
}

// Convert a scalar node to a boolean, double, or string (in that order)
// and for the first conversion that works, push it to the top of the JS stack.
inline JSValue pushYamlScalarAsJsPrimitive(JSContext& _js, const YAML::Node& _node) {
    bool booleanValue = false;
    float numberValue = 0.;
    if (getBool(_node, booleanValue))
        return _js.newBoolean(booleanValue);
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

inline JSValue parseNode(JSContext& _js, const YAML::Node& _node) {
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
                jsArray.setValueAtIndex(i, parseNode(_js, _node[i]));
            }
            return jsArray;
        }
        case YAML::NodeType::Map: {
            auto jsObject = _js.newObject();
            for (const auto& entry : _node) {
                if (!entry.first.IsScalar()) {
                    continue; // Can't put non-scalar keys in JS objects.
                }
                jsObject.setValueForProperty(entry.first.Scalar(), parseNode(_js, entry.second));
            }
            return jsObject;
        }
        default:
            return _js.newNull();
    }
}
