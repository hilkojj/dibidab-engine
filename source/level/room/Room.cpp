
#include "Room.h"

#include "../../ecs/systems/PlayerControlSystem.h"
#include "../../ecs/systems/SpawningSystem.h"
#include "../../ecs/systems/LuaScriptsSystem.h"
#include "../../ecs/components/Saving.dibidab.h"
#include "../../dibidab/component.h"

#include <gu/profiler.h>

void Room::initialize(Level *lvl)
{
    assert(lvl != nullptr);

    level = lvl;

    preLoadInitialize();
    loadPersistentEntities();
    postLoadInitialize();
}

void Room::preLoadInitialize()
{
    addSystem(new PlayerControlSystem("player control"));

    addSystem(new LuaScriptsSystem("lua functions"), true); // execute lua functions first, in case they might spawn entities, same reason as below:
    addSystem(new SpawningSystem("(de)spawning"), true); // SPAWN ENTITIES FIRST, so they get a chance to be updated before being rendered
    EntityEngine::initialize();
}

void Room::postLoadInitialize()
{
    afterLoad();
}

std::list<EntitySystem *> Room::getSystemsToUpdate() const
{
    std::list<EntitySystem *> toUpdate = EntityEngine::getSystemsToUpdate();
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

void Room::update(double deltaTime)
{
    gu::profiler::Zone roomZone("room " + std::to_string(getIndexInLevel()));

    EntityEngine::update(deltaTime);
}

bool Room::isLoadingPersistentEntities() const
{
    return bLoadingPersistentEntities;
}

void Room::setPersistent(bool bInPersistent)
{
    bIsPersistent = bInPersistent;
}

bool Room::isPersistent() const
{
    return bIsPersistent;
}

void Room::loadPersistentEntities()
{
    bLoadingPersistentEntities = true;

    const char *resolveFuncName = "resolvePersistentRef";
    const char *tryResolveFuncName = "tryResolvePersistentRef";
    sol::function originalResolvePersistentRef = luaEnvironment[resolveFuncName];
    sol::function originalTryResolvePersistentRef = luaEnvironment[tryResolveFuncName];
    assert(originalResolvePersistentRef.valid());
    assert(originalTryResolvePersistentRef.valid());
    auto tmpResolveFunc = [] (const sol::object &) {
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
            auto &p = entities.assign<Persistent>(e);
            p.persistentId = jsonEntity.at("persistentId");
            p.data = jsonEntity.at("data");

            if (jsonEntity.contains("name"))
            {
                std::string eName = jsonEntity["name"];
                setName(e, eName.c_str());
            }

            for (auto &[componentName, componentJson] : jsonEntity.at("components").items())
            {
                if (const dibidab::component_info *info = dibidab::findComponentInfo(componentName.c_str()))
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

int Room::getNumPersistentEntities() const
{
    return persistentEntitiesToLoad.is_array() ? persistentEntitiesToLoad.size() : 0;
}

void Room::persistentEntityToJson(entt::entity e, const Persistent &persistent, json &j) const
{
    j["persistentId"] = persistent.persistentId;
    j["template"] = persistent.applyTemplateOnLoad;
    j["data"] = persistent.data;

    if (persistent.saveName)
        if (const char *eName = getName(e))
            j["name"] = eName;

    json &componentsJson = j["components"] = json::object();

    for (auto &componentTypeName : persistent.saveComponents)
    {
        if (const dibidab::component_info *info = dibidab::findComponentInfo(componentTypeName.c_str()))
        {
            if (info->hasComponent(e, entities))
            {
                info->getJsonObject(e, entities, componentsJson[componentTypeName]);
            }
        }
    }
}

void Room::exportJsonData(json &j)
{
    events.emit(0, "BeforeSave");
    j = json{
        {"name", name},
        {"entities", revivableEntitiesToSave},
        {"persistentIdCounter", entities.ctx_or_set<PersistentEntities>().idCounter}
    };
    entities.view<const Persistent>().each([&](auto e, const Persistent &persistent) {

        j["entities"].push_back(json::object());
        persistentEntityToJson(e, persistent, j["entities"].back());
    });
}

void Room::loadJsonData(const json &j)
{
    name = j.at("name");
    persistentEntitiesToLoad = j.at("entities");
    entities.ctx_or_set<PersistentEntities>().idCounter = j.at("persistentIdCounter");
}
