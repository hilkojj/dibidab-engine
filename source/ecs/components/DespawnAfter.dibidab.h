#pragma once
#include <dibidab_header.h>

namespace dibidab::ecs
{
    struct DespawnAfter
    {
        dibidab_component;
        dibidab_expose(lua, json);
        float time = 0.0f;
        float timer = 0.0f;
    };
}
