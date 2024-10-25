
#include "Room.h"

#include "../../ecs/systems/PlayerControlSystem.h"
#include "../../ecs/systems/SpawningSystem.h"
#include "../../ecs/systems/LuaScriptsSystem.h"
#include "../../ecs/components/Saving.dibidab.h"
#include "../../ecs/templates/Template.h"
#include "../../reflection/ComponentInfo.h"

#include <gu/profiler.h>

void dibidab::level::Room::initialize(Level *lvl)
{
    assert(lvl != nullptr);

    level = lvl;

    preLoadInitialize();
    loadPersistentEntities();
    postLoadInitialize();
}

void dibidab::level::Room::preLoadInitialize()
{
    addSystem(new ecs::PlayerControlSystem("Player Control"));

    addSystem(new ecs::LuaScriptsSystem("Lua Functions"), true); // execute lua functions first, in case they might spawn entities, same reason as below:
    addSystem(new ecs::SpawningSystem("(De)spawning"), true); // SPAWN ENTITIES FIRST, so they get a chance to be updated before being rendered
    Engine::initialize();
}

void dibidab::level::Room::postLoadInitialize()
{
    afterLoad();
}

std::list<dibidab::ecs::System *> dibidab::level::Room::getSystemsToUpdate() const
{
    std::list<ecs::System *> toUpdate = Engine::getSystemsToUpdate();
    if (level != nullptr && level->isPaused())
    {
        auto it = toUpdate.begin();
        while (it != toUpdate.end())
        {
            if (systemsToUpdateDuringPause.find(*it) == systemsToUpdateDuringPause.end())
            {
                it = toUpdate.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    return toUpdate;
}

void dibidab::level::Room::update(double deltaTime)
{
    gu::profiler::Zone roomZone("room " + std::to_string(getIndexInLevel()));

    Engine::update(deltaTime);
}

bool dibidab::level::Room::isLoadingPersistentEntities() const
{
    return bLoadingPersistentEntities;
}

void dibidab::level::Room::setPersistent(bool bInPersistent)
{
    bIsPersistent = bInPersistent;
}

bool dibidab::level::Room::isPersistent() const
{
    return bIsPersistent;
}

void dibidab::level::Room::loadPersistentEntities()
{
    bLoadingPersistentEntities = true;

    const char *resolveFuncName = "resolvePersistentRef";
    const char *tryResolveFuncName = "tryResolvePersistentRef";
    sol::function originalResolvePersistentRef = luaEnvironment[resolveFuncName];
    sol::function originalTryResolvePersistentRef = luaEnvironment[tryResolveFuncName];
    assert(originalResolvePersistentRef.valid());
    assert(originalTryResolvePersistentRef.valid());
    auto tmpResolveFunc = [] (const sol::object &)
    {
        throw gu_err("You shouldn't try to resolve persistent entity references while loading persistent entities!");
    };
    luaEnvironment[resolveFuncName] = tmpResolveFunc;
    luaEnvironment[tryResolveFuncName] = tmpResolveFunc;

    for (json &jsonEntity : persistentEntitiesToLoad)
    {
        auto e = entities.create();
        assert(entities.valid(e));
        try
        {
            auto &p = entities.assign<ecs::Persistent>(e);
            p.persistentId = jsonEntity.at("persistentId");
            p.data = jsonEntity.at("data");

            if (jsonEntity.contains("name"))
            {
                std::string eName = jsonEntity["name"];
                setName(e, eName.c_str());
            }

            for (auto &[componentName, componentJson] : jsonEntity.at("components").items())
            {
                if (const dibidab::ComponentInfo *info = dibidab::findComponentInfo(componentName.c_str()))
                {
                    info->setFromJson(componentJson, e, entities);
                }
                else
                {
                    std::cerr << "Encountered non existing component '" << componentName << "' while loading entity:\n"
                        << jsonEntity.dump() << std::endl;
                }
            }

            std::string applyTemplate = jsonEntity.at("template");
            if (!applyTemplate.empty())
            {
                getTemplate(applyTemplate).createComponents(e, true);
            }
        }
        catch (std::exception &exc)
        {
            std::cerr << "Error while loading entity from JSON: \n" << exc.what() << std::endl;
            std::cerr << "entity json: " << jsonEntity.dump() << std::endl;
            entities.destroy(e);
        }
    }
    persistentEntitiesToLoad.clear();
    bLoadingPersistentEntities = false;
    luaEnvironment[resolveFuncName] = originalResolvePersistentRef;
    luaEnvironment[tryResolveFuncName] = originalTryResolvePersistentRef;
}

int dibidab::level::Room::getNumPersistentEntities() const
{
    return persistentEntitiesToLoad.is_array() ? persistentEntitiesToLoad.size() : 0;
}

void dibidab::level::Room::persistentEntityToJson(entt::entity e, const ecs::Persistent &persistent, json &j) const
{
    j["persistentId"] = persistent.persistentId;
    j["template"] = persistent.applyTemplateOnLoad;
    j["data"] = persistent.data;

    if (persistent.bSaveName)
        if (const char *eName = getName(e))
            j["name"] = eName;

    json &componentsJson = j["components"] = json::object();

    for (auto &componentTypeName : persistent.saveComponents)
    {
        if (const dibidab::ComponentInfo *info = dibidab::findComponentInfo(componentTypeName.c_str()))
        {
            if (info->hasComponent(e, entities))
            {
                info->getJsonObject(e, entities, componentsJson[componentTypeName]);
            }
        }
    }
}

void dibidab::level::Room::exportJsonData(json &j)
{
    events.emit(0, "BeforeSave");
    j = json {
        {"name", name},
        {"entities", revivableEntitiesToSave},
        {"persistentIdCounter", entities.ctx_or_set<ecs::PersistentEntities>().idCounter}
    };
    entities.view<const ecs::Persistent>().each([&](auto e, const ecs::Persistent &persistent)
    {
        j["entities"].push_back(json::object());
        persistentEntityToJson(e, persistent, j["entities"].back());
    });
}

void dibidab::level::Room::loadJsonData(const json &j)
{
    name = j.at("name");
    persistentEntitiesToLoad = j.at("entities");
    entities.ctx_or_set<ecs::PersistentEntities>().idCounter = j.at("persistentIdCounter");
}
