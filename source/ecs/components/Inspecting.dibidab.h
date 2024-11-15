#pragma once
#include "../../reflection/StructInspector.h"

#include <dibidab_header.h>

#include <math/math_utils.h>

#include <optional>
#include <map>

namespace dibidab
{
    struct ComponentInfo;
}

namespace dibidab::ecs
{
    struct Inspecting
    {
        dibidab_component;
        std::optional<vec2> nextWindowPos;
        std::map<const dibidab::ComponentInfo *, StructInspector> componentInspectors;
    };

    struct InspectingBehavior
    {
        dibidab_component;
    };
}
