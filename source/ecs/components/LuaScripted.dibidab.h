#pragma once
#include "../templates/LuaTemplate.h"

#include <dibidab_header.h>

namespace dibidab::ecs
{
    struct TimeoutFunc
    {
        sol::safe_function func;
        float timer;
    };


    struct LuaScripted
    {
      dibidab_component;
      dibidab_expose(lua, json);
        float updateAccumulator = 0.0f;
        float updateFrequency = 0.0f;

      dibidab_expose();
        LuaTemplate *usedTemplate = nullptr;

        sol::safe_function updateFunc;
        sol::safe_function onDestroyFunc;

        std::list<TimeoutFunc> timeoutFuncs;

        asset<luau::Script> updateFuncScript;
        asset<luau::Script> onDestroyFuncScript;

        sol::table saveData;
    };
}
