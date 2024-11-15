#pragma once
#include "../../behavior/Tree.h"

#include <dibidab_header.h>

namespace dibidab::ecs
{
    struct Behavior
    {
        dibidab_component;
        behavior::Tree tree;
    };

    struct BehaviorFinished
    {
        dibidab_component;
    };

    struct BehaviorAborted
    {
        dibidab_component;
    };
}
