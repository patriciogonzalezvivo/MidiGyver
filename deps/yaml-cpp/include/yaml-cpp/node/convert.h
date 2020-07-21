#pragma once

#include <array>
#include <limits>
#include <list>
#include <map>
#include <sstream>
#include <vector>

#include "yaml-cpp/binary.h"
#include "yaml-cpp/node/impl.h"
#include "yaml-cpp/node/iterator.h"
#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/type.h"
#include "yaml-cpp/null.h"

namespace YAML {
class Binary;
struct _Null;
template <typename T>
struct convert;
}  // namespace YAML

namespace YAML {
namespace conversion {
inline bool IsInfinity(const std::string& input) {
  return input == ".inf" || input == ".Inf" || input == ".INF" ||
         input == "+.inf" || input == "+.Inf" || input == "+.INF";
}

inline bool IsNegativeInfinity(const std::string& input) {
  return input == "-.inf" || input == "-.Inf" || input == "-.INF";
}

inline bool IsNaN(const std::string& input) {
  return input == ".nan" || input == ".NaN" || input == ".NAN";
}
}

// Node
template <>
struct convert<Node> {
  static void encode(const Node& node, Node& rhs) {
    rhs.reset(node);
  }

  static bool decode(const Node& node, Node& rhs) {
    rhs.reset(node);
    return true;
  }
};

// std::string
template <>
struct convert<std::string> {
  static void encode(const std::string& rhs, Node& node) {
    node.node().set_scalar(rhs);
  }

  static bool decode(const Node& node, std::string& rhs) {
    if (!node.IsScalar())
      return false;
    rhs = node.Scalar();
    return true;
  }
};

template <>
struct convert<detail::string_view> {
  static void encode(const detail::string_view& rhs, Node& node) {
    node.node().set_scalar(std::string(rhs.str, rhs.length));
  }
};

// C-strings can only be encoded
template <>
struct convert<const char*> {
  static void encode(const char*& rhs, Node& node) {
    node.node().set_scalar(rhs);
  }
};

template <std::size_t N>
struct convert<const char[N]> {
  static void encode(const char(&rhs)[N], Node& node) {
    node.node().set_scalar(rhs);
  }
};

template <>
struct convert<_Null> {
  static void encode(const _Null& /* rhs */, Node& node) {
    node.node().set_null();
  }

  static bool decode(const Node& node, _Null& /* rhs */) {
    return node.IsNull();
  }
};

#define YAML_DEFINE_CONVERT_STREAMABLE(type, negative_op)                \
  template <>                                                            \
  struct convert<type> {                                                 \
    static void encode(const type& rhs, Node& node) {                    \
      std::stringstream stream;                                          \
      stream.precision(std::numeric_limits<type>::digits10 + 1);         \
      stream << rhs;                                                     \
      return node.node().set_scalar(stream.str());                       \
    }                                                                    \
                                                                         \
    static bool decode(const Node& node, type& rhs) {                    \
      if (node.Type() != NodeType::Scalar)                               \
        return false;                                                    \
      const std::string& input = node.Scalar();                          \
      std::stringstream stream(input);                                   \
      stream.unsetf(std::ios::dec);                                      \
      if ((stream >> std::noskipws >> rhs) && (stream >> std::ws).eof()) \
        return true;                                                     \
      if (std::numeric_limits<type>::has_infinity) {                     \
        if (conversion::IsInfinity(input)) {                             \
          rhs = std::numeric_limits<type>::infinity();                   \
          return true;                                                   \
        } else if (conversion::IsNegativeInfinity(input)) {              \
          rhs = negative_op std::numeric_limits<type>::infinity();       \
          return true;                                                   \
        }                                                                \
      }                                                                  \
                                                                         \
      if (std::numeric_limits<type>::has_quiet_NaN &&                    \
          conversion::IsNaN(input)) {                                    \
        rhs = std::numeric_limits<type>::quiet_NaN();                    \
        return true;                                                     \
      }                                                                  \
                                                                         \
      return false;                                                      \
    }                                                                    \
  }

#define YAML_DEFINE_CONVERT_STREAMABLE_SIGNED(type) \
  YAML_DEFINE_CONVERT_STREAMABLE(type, -)

#define YAML_DEFINE_CONVERT_STREAMABLE_UNSIGNED(type) \
  YAML_DEFINE_CONVERT_STREAMABLE(type, +)

YAML_DEFINE_CONVERT_STREAMABLE_SIGNED(int);
YAML_DEFINE_CONVERT_STREAMABLE_SIGNED(short);
YAML_DEFINE_CONVERT_STREAMABLE_SIGNED(long);
YAML_DEFINE_CONVERT_STREAMABLE_SIGNED(long long);
YAML_DEFINE_CONVERT_STREAMABLE_UNSIGNED(unsigned);
YAML_DEFINE_CONVERT_STREAMABLE_UNSIGNED(unsigned short);
YAML_DEFINE_CONVERT_STREAMABLE_UNSIGNED(unsigned long);
YAML_DEFINE_CONVERT_STREAMABLE_UNSIGNED(unsigned long long);

YAML_DEFINE_CONVERT_STREAMABLE_SIGNED(char);
YAML_DEFINE_CONVERT_STREAMABLE_SIGNED(signed char);
YAML_DEFINE_CONVERT_STREAMABLE_UNSIGNED(unsigned char);

YAML_DEFINE_CONVERT_STREAMABLE_SIGNED(float);
YAML_DEFINE_CONVERT_STREAMABLE_SIGNED(double);
YAML_DEFINE_CONVERT_STREAMABLE_SIGNED(long double);

#undef YAML_DEFINE_CONVERT_STREAMABLE_SIGNED
#undef YAML_DEFINE_CONVERT_STREAMABLE_UNSIGNED
#undef YAML_DEFINE_CONVERT_STREAMABLE

// bool
template <>
struct convert<bool> {
  static void encode(bool rhs, Node& node) {
    node.node().set_scalar(rhs ? "true" : "false");
  }

  YAML_CPP_API static bool decode(const Node& node, bool& rhs);
};

// std::map
template <typename K, typename V>
struct convert<std::map<K, V>> {
  static void encode(const std::map<K, V>& rhs, Node& node) {

    node.node().set_type(NodeType::Map);

    for (typename std::map<K, V>::const_iterator it = rhs.begin();
         it != rhs.end(); ++it)
      node.force_insert(it->first, it->second);
  }

  static bool decode(const Node& node, std::map<K, V>& rhs) {
    if (!node.IsMap())
      return false;

    rhs.clear();
    for (const_iterator it = node.begin(); it != node.end(); ++it)
#if defined(__GNUC__) && __GNUC__ < 4
      // workaround for GCC 3:
      rhs[it->first.template as<K>()] = it->second.template as<V>();
#else
      rhs[it->first.as<K>()] = it->second.as<V>();
#endif
    return true;
  }
};

// std::vector
template <typename T>
struct convert<std::vector<T>> {
  static void encode(const std::vector<T>& rhs, Node& node) {
    node.node().set_type(NodeType::Sequence);

    for (typename std::vector<T>::const_iterator it = rhs.begin();
         it != rhs.end(); ++it)
      node.push_back(*it);
  }

  static bool decode(const Node& node, std::vector<T>& rhs) {
    if (!node.IsSequence())
      return false;

    rhs.clear();
    for (const_iterator it = node.begin(); it != node.end(); ++it)
#if defined(__GNUC__) && __GNUC__ < 4
      // workaround for GCC 3:
      rhs.push_back(it->template as<T>());
#else
      rhs.push_back(it->as<T>());
#endif
    return true;
  }
};

// std::list
template <typename T>
struct convert<std::list<T>> {
  static Node encode(const std::list<T>& rhs, Node& node) {
    node.node().set_type(NodeType::Sequence);

    for (typename std::list<T>::const_iterator it = rhs.begin();
         it != rhs.end(); ++it)
      node.push_back(*it);
    return node;
  }

  static bool decode(const Node& node, std::list<T>& rhs) {
    if (!node.IsSequence())
      return false;

    rhs.clear();
    for (const_iterator it = node.begin(); it != node.end(); ++it)
#if defined(__GNUC__) && __GNUC__ < 4
      // workaround for GCC 3:
      rhs.push_back(it->template as<T>());
#else
      rhs.push_back(it->as<T>());
#endif
    return true;
  }
};

// std::array
template <typename T, std::size_t N>
struct convert<std::array<T, N>> {
  static void encode(const std::array<T, N>& rhs, Node& node) {
    node.node().set_type(NodeType::Sequence);
    for (const auto& element : rhs) {
      node.push_back(element);
    }
  }

  static bool decode(const Node& node, std::array<T, N>& rhs) {
    if (!isNodeValid(node)) {
      return false;
    }

    for (auto i = 0u; i < node.size(); ++i) {
#if defined(__GNUC__) && __GNUC__ < 4
      // workaround for GCC 3:
      rhs[i] = node[i].template as<T>();
#else
      rhs[i] = node[i].as<T>();
#endif
    }
    return true;
  }

 private:
  static bool isNodeValid(const Node& node) {
    return node.IsSequence() && node.size() == N;
  }
};

// std::pair
template <typename T, typename U>
struct convert<std::pair<T, U>> {
  static void encode(const std::pair<T, U>& rhs, Node& node) {
    node.node().set_type(NodeType::Sequence);
    node.push_back(rhs.first);
    node.push_back(rhs.second);
  }

  static bool decode(const Node& node, std::pair<T, U>& rhs) {
    if (!node.IsSequence())
      return false;
    if (node.size() != 2)
      return false;

#if defined(__GNUC__) && __GNUC__ < 4
    // workaround for GCC 3:
    rhs.first = node[0].template as<T>();
#else
    rhs.first = node[0].as<T>();
#endif
#if defined(__GNUC__) && __GNUC__ < 4
    // workaround for GCC 3:
    rhs.second = node[1].template as<U>();
#else
    rhs.second = node[1].as<U>();
#endif
    return true;
  }
};

// binary
template <>
struct convert<Binary> {
  static void encode(const Binary& rhs, Node& node) {
    node.node().set_scalar(EncodeBase64(rhs.data(), rhs.size()));
  }

  static bool decode(const Node& node, Binary& rhs) {
    if (!node.IsScalar())
      return false;

    std::vector<unsigned char> data = DecodeBase64(node.Scalar());
    if (data.empty() && !node.Scalar().empty())
      return false;

    rhs.swap(data);
    return true;
  }
};
}
