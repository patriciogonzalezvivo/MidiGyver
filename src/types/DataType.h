#pragma once

#include "../ops/strings.h"

enum DataType {
    TYPE_UNKNOWN,

    TYPE_BUTTON,
    TYPE_TOGGLE,
    
    TYPE_NUMBER,
    
    TYPE_VECTOR,
    TYPE_COLOR,
    
    TYPE_STRING,

    TYPE_MIDI_NOTE,
    TYPE_MIDI_CONTROLLER_CHANGE,
    TYPE_MIDI_TIMING_TICK
};


inline std::string toString(const DataType& _type ) {
    static std::string typeNames[] = { 
        "UNKNOWN",
        "BUTTON",
        "TOGGLE",
        "NUMBER",
        "VECTOR",
        "COLOR",
        "STRING",
        "NOTE",
        "CONTROLLER_CHANGE",
        "TIMING_TICK"
    };

    return typeNames[(size_t)_type];
}

inline DataType toDataType(const std::string& _string ) {
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
