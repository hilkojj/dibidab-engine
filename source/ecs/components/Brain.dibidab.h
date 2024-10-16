#pragma once
#include "../../ai/behavior_trees/BehaviorTree.h"

#include <dibidab_header.h>

struct Brain
{
  dibidab_component;
    BehaviorTree behaviorTree;
};
