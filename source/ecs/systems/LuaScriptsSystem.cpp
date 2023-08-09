
#include "LuaScriptsSystem.h"

// https://github.com/skypjack/entt/issues/17

void LuaScriptsSystem::init(EntityEngine *room)
{
    engine = room;
    room->entities.on_destroy<LuaScripted>().connect<&LuaScriptsSystem::onDestroyed>(this);
}

void LuaScriptsSystem::update(double deltaTime, EntityEngine *room)
{
    std::vector<std::tuple<double, entt::entity, sol::safe_function>> updatesToCall;
    std::vector<std::pair<entt::entity, sol::safe_function>> timeoutsToCall;

    room->entities.view<LuaScripted>().each([&](auto e, LuaScripted &scripted)
    {
        if (scripted.updateFunc.lua_state() && scripted.updateFunc.valid() && !scripted.updateFunc.is<sol::nil_t>())
        {
            if (scripted.updateFrequency <= 0)
            {
                updatesToCall.emplace_back(deltaTime, e, scripted.updateFunc);
            }
            else
            {
                scripted.updateAccumulator += deltaTime;
                if (scripted.updateAccumulator >= scripted.updateFrequency)
                {
                    scripted.updateAccumulator -= scripted.updateFrequency;
                    updatesToCall.emplace_back(scripted.updateFrequency, e, scripted.updateFunc);
                    // NOTE: removed support for being called multiple times per frame
                    // because the update function might change during one update, invalidating the updatesToCall list.
                    // Also removed passing the saveData.
                }
            }
        }

        auto it = scripted.timeoutFuncs.begin();
        while (it != scripted.timeoutFuncs.end())
        {
            auto &f = *it;
            f.timer -= deltaTime;

            if (f.timer < 0)
            {
                timeoutsToCall.emplace_back(e, f.func);
                it = scripted.timeoutFuncs.erase(it);
            }
            else ++it;
        }
    });

    for (auto &[updateTimeDelta, entity, function] : updatesToCall)
    {
        if (room->entities.valid(entity))
        {
            luau::tryCallFunction(function, updateTimeDelta, entity);
        }
    }
    for (auto &[entity, function] : timeoutsToCall)
    {
        if (room->entities.valid(entity))
        {
            luau::tryCallFunction(function, entity);
        }
    }
}

void LuaScriptsSystem::onDestroyed(entt::registry &reg, entt::entity e)
{
    LuaScripted &scripted = reg.get<LuaScripted>(e);
    if (!scripted.onDestroyFunc.lua_state() || !scripted.onDestroyFunc.valid() || scripted.onDestroyFunc.is<sol::nil_t>())
        return;

    try
    {
        sol::safe_function cpy = scripted.onDestroyFunc;
        sol::table saveData = scripted.saveData;
        // cpy and saveData are copied because EnTT might move them if the lua script causes a `LuaScripted` component to be created

        luau::callFunction(cpy, e, saveData);
    }
    catch (std::exception &exc)
    {
        assert(scripted.onDestroyFuncScript.isSet());
        std::cerr << "Error while calling Lua onDestroy callback for entity#" << int(e) << " (" << scripted.onDestroyFuncScript.getLoadedAsset().fullPath << "):" << std::endl;
        std::cerr << exc.what() << std::endl;
    }
}

LuaScriptsSystem::~LuaScriptsSystem()
{
    engine->entities.view<LuaScripted>().each([&] (auto e, auto) {
        onDestroyed(engine->entities, e);
    });
}
