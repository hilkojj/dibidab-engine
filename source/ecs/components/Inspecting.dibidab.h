#pragma once
#include "../../dibidab/utils/StructEditor.h"

#include <dibidab_header.h>

#include <math/math_utils.h>

#include <optional>
#include <map>

namespace dibidab
{
    struct component_info;
}

class StructEditor;


struct Inspecting
{
  dibidab_component;
    std::optional<vec2> nextWindowPos;
    std::map<const dibidab::component_info *, StructEditor> componentEditors;
};

struct InspectingBrain
{
  dibidab_component;
};
