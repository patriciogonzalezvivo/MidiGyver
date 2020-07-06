#pragma once

#include <iostream>     // std::cout, std::fixed
#include <iomanip>      // std::setprecision

#include "yaml-cpp/yaml.h"
#include "tools.h"

struct Color { 
    Color(): r(0.0f), g(0.0f), b(0.0f), a(1.0f) {};
    Color(float _r, float _g, float _b): r(_r), g(_g), b(_b), a(1.0f) {};
    Color(float _r, float _g, float _b, float _a): r(_r), g(_g), b(_b), a(_a) {};
    float r, g, b, a;
};

inline bool operator==(const Color& lhs, const Color& rhs) {
    return (lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b);
}

inline Color lerp(const Color& start, const Color& stop, float amt) {
	return Color( start.r + (stop.r - start.r) * amt,
                start.g + (stop.g - start.g) * amt,
                start.b + (stop.b - start.b) * amt,
                start.a + (stop.a - start.a) * amt );
}

inline std::ostream& operator<<(std::ostream& strm, const Color& c) {
    strm << std::setprecision(3);
    strm << c.r << "," << c.g << "," << c.b << "," << c.a;
    return strm;
}

namespace YAML {
template<>
struct convert<Color> {
    static Node encode(const Color& rhs) {
      Node node;
      node.push_back(rhs.r);
      node.push_back(rhs.g);
      node.push_back(rhs.b);
      node.push_back(rhs.a);
      return node;
    }

    static bool decode(const Node& node, Color& rhs) {
        if(!node.IsSequence() || node.size() < 3)
            return false;

        rhs.r = node[0].as<float>();
        rhs.g = node[1].as<float>();
        rhs.b = node[2].as<float>();

        if (node.size() == 4)
          rhs.a = node[3].as<float>();
          
        return true;
    }
};
}