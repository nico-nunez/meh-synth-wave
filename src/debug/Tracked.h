#pragma once

#include <iostream>
#include <typeinfo>

namespace Synth {
namespace Debug {

template <typename T> struct Tracked {
  T value;

  Tracked() = default;

  Tracked(const T& v) : value(v) {
    std::cout << "Tracked<" << typeid(T).name() << "> constructed\n";
  }

  Tracked(const Tracked& other) : value(other.value) {
    std::cout << "Tracked<" << typeid(T).name() << "> COPIED\n";
  }

  Tracked& operator=(const Tracked& other) {
    value = other.value;
    std::cout << "Tracked<" << typeid(T).name() << "> COPY assigned\n";
    return *this;
  }

  Tracked(Tracked&& other) noexcept : value(std::move(other.value)) {
    std::cout << "Tracked<" << typeid(T).name() << "> MOVED\n";
  }

  Tracked& operator=(Tracked&& other) noexcept {
    value = std::move(other.value);
    std::cout << "Tracked<" << typeid(T).name() << "> MOVE assigned\n";
    return *this;
  }

  // Implicit conversions for easy use
  operator T&() { return value; }
  operator const T&() const { return value; }

  // Access underlying value
  T& get() { return value; }
  const T& get() const { return value; }
};

} // namespace Debug
} // namespace Synth
