#pragma once

#include <array>

namespace YAML {
namespace Exp {

//http://stackoverflow.com/questions/39058850/how-to-align-stdarray-contained-data
template<std::size_t N>
struct alignas(sizeof(std::size_t)) Source : public std::array<char,N> {};

using StreamSource = Source<8>;

}
}
