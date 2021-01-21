#pragma once

#include <iostream>     // std::cout, std::fixed
#include <iomanip>      // std::setprecision

#include "yaml-cpp/yaml.h"
#include "../ops/values.h"


// Code from https://bottosson.github.io/posts/oklab/
// By Björn Ottosson
inline float linear2rgb(float x) {
    if (x >= 0.0031308)
        return (1.055f) * pow(x, 0.416666667f) - 0.055f;
    else
        return 12.92f * x;
}

// Code from https://bottosson.github.io/posts/oklab/
// By Björn Ottosson
inline float rgb2linear(float x) {
    if (x >= 0.04045)
        return pow((x + 0.055f)/(1 + 0.055f) , 2.4f);
    else 
        return x * 0.077279753f;
}

struct Color { 
    Color(): r(0.0f), g(0.0f), b(0.0f), a(1.0f) {};
    Color(float _r, float _g, float _b): r(_r), g(_g), b(_b), a(1.0f) {};
    Color(float _r, float _g, float _b, float _a): r(_r), g(_g), b(_b), a(_a) {};

    Color toLinear() const { return Color(rgb2linear(r), rgb2linear(g), rgb2linear(b), a); }
    Color toSRGB() const { return Color(linear2rgb(r), linear2rgb(g), linear2rgb(b), a); }

    float r, g, b, a;
};

// Code from https://bottosson.github.io/posts/oklab/
// By Björn Ottosson
struct RGB { float r, g, b; };
struct LAB { float L, a, b; };

inline LAB linear2oklab(Color c)  {
    float l = 0.4121656120f * c.r + 0.5362752080f * c.g + 0.0514575653f * c.b;
    float m = 0.2118591070f * c.r + 0.6807189584f * c.g + 0.1074065790f * c.b;
    float s = 0.0883097947f * c.r + 0.2818474174f * c.g + 0.6302613616f * c.b;

    float l_ = cbrtf(l);
    float m_ = cbrtf(m);
    float s_ = cbrtf(s);

    return {
        0.2104542553f*l_ + 0.7936177850f*m_ - 0.0040720468f*s_,
        1.9779984951f*l_ - 2.4285922050f*m_ + 0.4505937099f*s_,
        0.0259040371f*l_ + 0.7827717662f*m_ - 0.8086757660f*s_,
    };
}

// Code from https://bottosson.github.io/posts/oklab/
// By Björn Ottosson
inline Color oklab2linear(LAB c, float alpha) {
    float l_ = c.L + 0.3963377774f * c.a + 0.2158037573f * c.b;
    float m_ = c.L - 0.1055613458f * c.a - 0.0638541728f * c.b;
    float s_ = c.L - 0.0894841775f * c.a - 1.2914855480f * c.b;

    float l = l_*l_*l_;
    float m = m_*m_*m_;
    float s = s_*s_*s_;

    return {
        + 4.0767245293f*l - 3.3072168827f*m + 0.2307590544f*s,
        - 1.2681437731f*l + 2.6093323231f*m - 0.3411344290f*s,
        - 0.0041119885f*l - 0.7034763098f*m + 1.7068625689f*s,
        alpha
    };
}

inline LAB lerp(const LAB& A, const LAB& B, float amt) {
    return {A.L + (B.L -  A.L) * amt,
            A.a + (B.a -  A.a) * amt,
            A.b + (B.b -  A.b) * amt};
}


inline bool operator==(const Color& lhs, const Color& rhs) {
    return (lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b);
}

inline Color lerp(const Color& A, const Color& B, float amt) {
    LAB LAB_A = linear2oklab(A.toLinear());
    LAB LAB_B = linear2oklab(B.toLinear());
    LAB LAB_mix = lerp(LAB_A, LAB_B, amt);
    Color linear_mix = oklab2linear(LAB_mix, A.a + (B.a -  A.a) * amt);
    return linear_mix.toSRGB();

// 	return Color( A.r + (B.r - A.r) * amt,
//                 A.g + (B.g - A.g) * amt,
//                 A.b + (B.b - A.b) * amt,
//                 A.a + (B.a - A.a) * amt );
}

inline std::ostream& operator<<(std::ostream& strm, const Color& c) {
    strm << std::setprecision(3);
    strm << c.r << "," << c.g << "," << c.b << "," << c.a;
    return strm;
}

inline YAML::Emitter& operator << (YAML::Emitter& out, const Color& c) {
	out << YAML::Flow;
	out << YAML::BeginSeq << c.r << c.g << c.b << c.a << YAML::EndSeq;
	return out;
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