#pragma once
#include "../../behavior/Tree.h"

#include <dibidab_header.h>

namespace dibidab::ecs
{
    struct Brain
    {
        dibidab_component;
        behavior::Tree behaviorTree;
    };
}
