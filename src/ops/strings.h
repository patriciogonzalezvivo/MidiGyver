#pragma once

#include <string>
#include <sstream>
#include <iomanip> 
#include <algorithm>
#include <functional>

template <class T>
inline std::string toString(const T& _value){
    std::ostringstream out;
    out << _value;
    return out.str();
}


/// like sprintf "%4f" format, in this example precision=4
template <class T>
inline std::string toString(const T& _value, int _precision) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(_precision) << _value;
    return out.str();
}

/// like sprintf "% 4d" or "% 4f" format, in this example width=4, fill=' '
template <class T>
inline std::string toString(const T& _value, int _width, char _fill) {
    std::ostringstream out;
    out << std::fixed << std::setfill(_fill) << std::setw(_width) << _value;
    return out.str();
}

/// like sprintf "%04.2d" or "%04.2f" format, in this example precision=2, width=4, fill='0'
template <class T>
inline std::string toString(const T& _value, int _precision, int _width, char _fill) {
    std::ostringstream out;
    out << std::fixed << std::setfill(_fill) << std::setw(_width) << std::setprecision(_precision) << _value;
    return out.str();
}

inline int toInt(const std::string& _string) {
    int x = 0;
    std::istringstream cur(_string);
    cur >> x;
    return x;
}

inline float toFloat(const std::string& _string) {
    float x = 0;
    std::istringstream cur(_string);
    cur >> x;
    return x;
}

inline double toDouble(const std::string& _string) {
    double x = 0;
    std::istringstream cur(_string);
    cur >> x;
    return x;
}

inline bool match(const char* _pattern, const char* _target) { 
    // If we reach at the end of both strings, we are done 
    if (*_pattern == '\0' && *_target == '\0') 
        return true; 
  
    // Make sure that the characters after '*' are present 
    // in _target string. This function assumes that the _pattern 
    // string will not contain two consecutive '*' 
    if (*_pattern == '*' && *(_pattern+1) != '\0' && *_target == '\0') 
        return false; 
  
    // If the _pattern string contains '?', or current characters 
    // of both strings match 
    if (*_pattern == '?' || *_pattern == *_target) 
        return match(_pattern+1, _target+1); 
  
    // If there is *, then there are two possibilities 
    // a) We consider current character of _target string 
    // b) We ignore current character of _target string. 
    if (*_pattern == '*') 
        return match(_pattern+1, _target) || match(_pattern, _target+1); 
    return false; 
} 

inline bool beginsWith(const std::string& _stringA, const std::string& _stringB) {
    for (size_t i = 0; i < _stringB.size(); i++) {
        if (_stringB[i] != _stringA[i]) {
            return false;
        }
    }
    return true;
}

inline void stringReplace(std::string& str, char rep) {
#if _WIN32
    replace_if(str.begin(), str.end(), std::bind2nd<std::equal_to<char>>(std::equal_to<char>(), ' '), rep);
    replace_if(str.begin(), str.end(), std::bind2nd<std::equal_to<char>>(std::equal_to<char>(), ':'), rep);
    replace_if(str.begin(), str.end(), std::bind2nd<std::equal_to<char>>(std::equal_to<char>(), ','), rep);
    replace_if(str.begin(), str.end(), std::bind2nd<std::equal_to<char>>(std::equal_to<char>(), '/'), rep);
#else
	replace_if(str.begin(), str.end(), std::bind2nd(std::equal_to<char>(), ' '), rep);
	replace_if(str.begin(), str.end(), std::bind2nd(std::equal_to<char>(), ':'), rep);
	replace_if(str.begin(), str.end(), std::bind2nd(std::equal_to<char>(), ','), rep);
	replace_if(str.begin(), str.end(), std::bind2nd(std::equal_to<char>(), '/'), rep);
#endif
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