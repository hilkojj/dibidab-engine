
#include "LuaScriptsSystem.h"

// https://github.com/skypjack/entt/issues/17

void LuaScriptsSystem::init(EntityEngine *room)
{
    engine = room;
    room->entities.on_destroy<LuaScripted>().connect<&LuaScriptsSystem::onDestroyed>(this);
}

void LuaScriptsSystem::update(double deltaTime, EntityEngine *room)
{
    room->entities.view<LuaScripted>().each([&](auto e, LuaScripted &scripted) {

        auto scriptedPtr = &scripted;

        if (scripted.updateFrequency <= 0)
            callUpdateFunc(e, scripted, deltaTime);
        else
        {
            scripted.updateAccumulator += deltaTime;
            while (scriptedPtr != NULL && scriptedPtr->updateAccumulator > scriptedPtr->updateFrequency)
            {
                scriptedPtr->updateAccumulator -= scriptedPtr->updateFrequency;
                callUpdateFunc(e, *scriptedPtr, scriptedPtr->updateFrequency);
                if (!room->entities.valid(e))
                    return;
                else
                    scriptedPtr = room->entities.try_get<LuaScripted>(e); // ptr might have been moved by EnTT/lua script
            }
        }

        // ONLY USE scriptedPtr NOW!!!!

        std::vector<sol::safe_function> call;

        auto it = scriptedPtr->timeoutFuncs.begin();
        while (it != scriptedPtr->timeoutFuncs.end())
        {
            auto &f = *it;
            f.timer -= deltaTime;

            if (f.timer < 0)
            {
                call.push_back(f.func);
                it = scriptedPtr->timeoutFuncs.erase(it);
            }
            else ++it;
        }

        for (auto &func : call)
        {
            luau::callFunction(func, e);
            if (!room->entities.valid(e))
                return;
        }
    });
}

void LuaScriptsSystem::callUpdateFunc(entt::entity e, LuaScripted &scripted, float deltaTime)
{
    if (!scripted.updateFunc.lua_state() || !scripted.updateFunc.valid() || scripted.updateFunc.is<sol::nil_t>())
        return;

    try
    {
        sol::safe_function cpy = scripted.updateFunc;
        sol::table saveData = scripted.saveData;
        // cpy and saveData are copied because EnTT might move them if the lua script causes a `LuaScripted` component to be created

        luau::callFunction(cpy, deltaTime, e, saveData);
    }
    catch (std::exception &exc)
    {
        assert(scripted.updateFuncScript.isSet());  // todo, `scripted` might be an invalid reference at this point
        std::cerr << "Error while calling Lua update function for entity#" << int(e) << " (" << scripted.updateFuncScript.getLoadedAsset().fullPath << "):" << std::endl;
        std::cerr << exc.what() << std::endl;
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
