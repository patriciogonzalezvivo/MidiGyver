#pragma once

#include <string>
#include <algorithm>

template <class T>
inline std::string toString(const T& _value){
    std::ostringstream out;
    out << _value;
    return out.str();
}

inline void stringReplace(std::string& str, char rep) {
    replace_if(str.begin(), str.end(), std::bind2nd(std::equal_to<char>(), ' '), rep);
    replace_if(str.begin(), str.end(), std::bind2nd(std::equal_to<char>(), ':'), rep);
    replace_if(str.begin(), str.end(), std::bind2nd(std::equal_to<char>(), ','), rep);
    replace_if(str.begin(), str.end(), std::bind2nd(std::equal_to<char>(), '/'), rep);
}

inline std::vector<std::string> split(const std::string& _string, char _sep, bool _tolerate_empty) {
    std::vector<std::string> tokens;
    std::size_t start = 0, end = 0;
    while ((end = _string.find(_sep, start)) != std::string::npos) {
        if (end != start || _tolerate_empty) {
          tokens.push_back(_string.substr(start, end - start));
        }
        start = end + 1;
    }
    if (end != start || _tolerate_empty) {
       tokens.push_back(_string.substr(start));
    }
    return tokens;
}

inline float map(float value, float inputMin, float inputMax, float outputMin, float outputMax) {
    float outVal = ((value - inputMin) / (inputMax - inputMin) * (outputMax - outputMin) + outputMin);
    return outVal;
}

inline float lerp(float start, float stop, float amt) {
	return start + (stop-start) * amt;
}
