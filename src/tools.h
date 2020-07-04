#pragma once

#include <string>
#include <algorithm>

inline void stringReplace(std::string& str, char rep) {
    replace_if(str.begin(), str.end(), std::bind2nd(std::equal_to<char>(), ' '), rep);
    replace_if(str.begin(), str.end(), std::bind2nd(std::equal_to<char>(), ':'), rep);
}

inline float map(float value, float inputMin, float inputMax, float outputMin, float outputMax) {
    float outVal = ((value - inputMin) / (inputMax - inputMin) * (outputMax - outputMin) + outputMin);
    return outVal;
}

template <class T>
inline std::string toString(const T& _value){
    std::ostringstream out;
    out << _value;
    return out.str();
}