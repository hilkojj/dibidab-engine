
#include "../../game/SaveGame.h"
#include "LuaEntityTemplate.h"
#include "../components/LuaScripted.dibidab.h"

#include <asset_manager/AssetManager.h>
#include <utils/string_utils.h>


LuaEntityTemplate::LuaEntityTemplate(const char *assetName, const char *name, EntityEngine *engine_)
    : script(assetName), name(name),
      luaEnvironment(engine_->luaEnvironment.lua_state(), sol::create, engine_->luaEnvironment)
{
    this->engine = engine_; // DONT RENAME engine_ to engine!!!, lambdas should use this->engine.
    luaEnvironment["TEMPLATE_NAME"] = name;
    luaEnvironment["TEMPLATE_PTR"] = this;

    defaultArgs = sol::table::create(luaEnvironment.lua_state());

    int
        TEMPLATE = luaEnvironment["TEMPLATE"] = 1 << 0,
        ARGS = luaEnvironment["ARGS"] = 1 << 1;

    auto setPersistentMode = [&](int mode, sol::optional<std::vector<std::string>> componentsToSave) {

        persistency = Persistent();
        if (mode & TEMPLATE)
            persistency.applyTemplateOnLoad = this->name;   // 'name' is a pointer and thus can break. 'this->name' is a string :)

        bPersistentArgs = mode & ARGS;
        if (componentsToSave.has_value())
        {
            persistency.saveComponents = componentsToSave.value();
        }
    };
    luaEnvironment["persistenceMode"] = setPersistentMode;
    setPersistentMode(TEMPLATE | ARGS, sol::optional<std::vector<std::string>>());

    luaEnvironment["defaultArgs"] = [&](const sol::table &table) {
        defaultArgs = table;
    };
    luaEnvironment["description"] = [&](const char *d) {
        description = d;
    };
    luaEnvironment["setUpdateFunction"] = [&](entt::entity entity, float updateFrequency, const sol::safe_function &func, sol::optional<bool> randomAcummulationDelay) {

        LuaScripted &scripted = engine->entities.get_or_assign<LuaScripted>(entity);
        scripted.updateFrequency = updateFrequency;

        if (randomAcummulationDelay.value_or(true))
            scripted.updateAccumulator = scripted.updateFrequency * mu::random();
        else
            scripted.updateAccumulator = 0;

        scripted.updateFunc = func;
        scripted.updateFuncScript = script;
    };
    luaEnvironment["setOnDestroyCallback"] = [&](entt::entity entity, const sol::safe_function &func) {

        LuaScripted &scripted = engine->entities.get_or_assign<LuaScripted>(entity);
        scripted.onDestroyFunc = func;
        scripted.onDestroyFuncScript = script;
    };

    runScript();
}

void LuaEntityTemplate::runScript()
{
    try
    {
        // todo: use same lua_state as 'env' is in

        sol::protected_function_result result = luau::getLuaState().safe_script(script->getByteCode().as_string_view(), luaEnvironment);
        if (!result.valid())
            throw gu_err(result.get<sol::error>().what());

        luaCreateComponents = luaEnvironment["create"];
        if (!luaCreateComponents.valid())
            throw gu_err("No create() function found!");
    }
    catch (std::exception &e)
    {
        std::cerr << "Error while running template script " << script.getLoadedAsset()->fullPath << ":" << std::endl;
        std::cerr << e.what() << std::endl;
    }
}

void LuaEntityTemplate::createComponents(entt::entity e, bool persistent)
{
    auto *p = persistent ? engine->entities.try_get<Persistent>(e) : nullptr;
    if (p)
        createComponentsWithJsonArguments(e, p->data, true);
    else
        createComponentsWithLuaArguments(e, sol::optional<sol::table>(), persistent);
}

void LuaEntityTemplate::createComponentsWithLuaArguments(entt::entity e, sol::optional<sol::table> arguments, bool persistent)
{
    if (script.hasReloaded())
        runScript();

    try
    {
        if (arguments.has_value() && defaultArgs.valid())
        {
            for (auto &[key, defaultVal] : defaultArgs)
            {
                if (!arguments.value()[key].valid())
                    arguments.value()[key] = defaultVal;
            }
        } else arguments = defaultArgs;

        LuaScripted& luaScripted = engine->entities.get_or_assign<LuaScripted>(e);
        if (luaScripted.usedTemplate == nullptr)
        {
            luaScripted.usedTemplate = this;
        }

#ifndef DIBIDAB_NO_SAVE_GAME
        std::string id;
        if (!luaScripted.saveData.valid())
        {
            id = arguments.value()["saveGameEntityID"].get_or<std::string, std::string>(getUniqueID());
            luaScripted.saveData = SaveGame::getSaveDataForEntity(id, !persistent);
        }
#endif

        if (persistent)
        {
            PersistentEntityID persistentEntityID;
            std::string previousAppliedTemplate;
            if (const Persistent *pOld = engine->entities.try_get<Persistent>(e))
            {
                persistentEntityID = pOld->persistentId;
                previousAppliedTemplate = pOld->applyTemplateOnLoad;
            }
            else
            {
                persistentEntityID = ++engine->entities.ctx_or_set<PersistentEntities>().idCounter;
            }
            engine->entities.ctx_or_set<PersistentEntities>().persistentEntityIdMap[persistentEntityID] = e;

            auto &p = engine->entities.assign_or_replace<Persistent>(e, persistency);
            if (!previousAppliedTemplate.empty())
            {
                p.applyTemplateOnLoad = previousAppliedTemplate;
            }
            p.persistentId = persistentEntityID;
            if (bPersistentArgs && arguments.has_value() && arguments.value().valid())
                jsonFromLuaTable(arguments.value(), p.data);

            assert(p.data.is_object());
#ifndef DIBIDAB_NO_SAVE_GAME
            if (!id.empty())
                p.data["saveGameEntityID"] = id;
#endif
        }

        sol::protected_function_result result = luaCreateComponents(
            e,
            arguments,
            persistent
#ifndef DIBIDAB_NO_SAVE_GAME
            ,
            luaScripted.saveData
#endif
        );
        if (!result.valid())
            throw gu_err(result.get<sol::error>().what());
        // NOTE!!: ALL REFERENCES TO COMPONENTS MIGHT BE BROKEN AFTER CALLING createFunc. (EnTT might resize containers)
    }
    catch (std::exception &e)
    {
        std::cerr << "Error while creating entity using " << script.getLoadedAsset()->fullPath << ":" << std::endl;
        std::cerr << e.what() << std::endl;
    }
}

const std::string &LuaEntityTemplate::getDescription()
{
    if (script.hasReloaded())
        runScript();
    return description;
}

json LuaEntityTemplate::getDefaultArgs()
{
    if (script.hasReloaded())
        runScript();
    json j;
    jsonFromLuaTable(defaultArgs, j);
    return j;
}

void LuaEntityTemplate::createComponentsWithJsonArguments(entt::entity e, const json &arguments, bool persistent)
{
    auto table = sol::table::create(luaEnvironment.lua_state());
    if (arguments.is_structured())
        jsonToLuaTable(table, arguments);
    createComponentsWithLuaArguments(e, table, persistent);
}

std::string LuaEntityTemplate::getUniqueID()
{
    return name + "_" + su::randomAlphanumeric(24);
}

sol::environment &LuaEntityTemplate::getTemplateEnvironment()
{
    return luaEnvironment;
}
