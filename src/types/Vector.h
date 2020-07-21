#pragma once

#include <iostream>     // std::cout, std::fixed
#include <iomanip>      // std::setprecision

#include "yaml-cpp/yaml.h"
#include "../ops/values.h"

struct Vector { 
    Vector(): x(0.0f), y(0.0f), z(0.0f) {};
    Vector(float _x, float _y): x(_x), y(_y), z(0.0) {};
    Vector(float _x, float _y, float _z): x(_x), y(_y), z(_z) {};
    float x, y, z;
};

inline bool operator==(const Vector& lhs, const Vector& rhs) {
    return (lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z);
}

inline Vector lerp(const Vector& start, const Vector& stop, float amt) {
	return Vector(  start.x + (stop.x - start.x) * amt,
                    start.y + (stop.y - start.y) * amt,
                    start.z + (stop.z - start.z) * amt);
}

inline std::ostream& operator<<(std::ostream& strm, const Vector& v) {
    strm << std::setprecision(3);
    strm << v.x << "," << v.y << "," << v.z;
    return strm;
}

inline YAML::Emitter& operator << (YAML::Emitter& out, const Vector& v) {
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
	return out;
}

namespace YAML {
template<>
struct convert<Vector> {
    static Node encode(const Vector& rhs) {
      Node node;
      node.push_back(rhs.x);
      node.push_back(rhs.y);
      node.push_back(rhs.z);
      return node;
    }

    static bool decode(const Node& node, Vector& rhs) {
        if(!node.IsSequence() || node.size() < 2)
            return false;

        rhs.x = node[0].as<float>();
        rhs.y = node[1].as<float>();

        if (node.size() == 3)
            rhs.z = node[2].as<float>();
          
        return true;
    }
};
}