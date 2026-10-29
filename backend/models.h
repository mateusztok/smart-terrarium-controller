#pragma once

#include <array>

namespace terrarium {

struct SetpointField {
  const char *name;
  const char *unit;
};

inline constexpr std::array<SetpointField, 3> kSetpointFields = {{
    {"temperature", "°C"},
    {"humidity", "%"},
    {"light", "lux"},
}};

} // namespace terrarium
