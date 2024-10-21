#pragma once
#include <dibidab_header.h>

#include <math/math_utils.h>

#include <optional>

struct Inspecting
{
  dibidab_component;
    std::optional<vec2> nextWindowPos;
};


struct InspectingBrain
{
  dibidab_component;
};
