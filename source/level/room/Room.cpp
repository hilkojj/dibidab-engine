
#include "Room.h"

#include "../../ecs/systems/PlayerControlSystem.h"
#include "../../ecs/systems/AudioSystem.h"
#include "../../ecs/systems/SpawningSystem.h"
#include "../../ecs/systems/LuaScriptsSystem.h"

#include "../../generated/Saving.hpp"

#include <gu/profiler.h>

void Room::initialize(Level *lvl)
{
    assert(lvl != NULL);

    level = lvl;

    preLoadInitialize();
    loadPersistentEntities();
    postLoadInitialize();
}

void Room::preLoadInitialize()
{
    addSystem(new PlayerControlSystem("player control"));
    addSystem(new AudioSystem("audio"));

    addSystem(new LuaScriptsSystem("lua functions"), true); // execute lua functions first, in case they might spawn entities, same reason as below:
    addSystem(new SpawningSystem("(de)spawning"), true); // SPAWN ENTITIES FIRST, so they get a chance to be updated before being rendered
    EntityEngine::initialize();

    // THIS on_destroy() SHOULD STAY HERE (after EntityEngine::initialize()) OTHERWISE CALLBACK WILL BE CALLED AFTER `Named`-component (or other components) ARE ALREADY REMOVED!
    entities.on_destroy<Persistent>().connect<&Room::tryToSaveRevivableEntity>(this);
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
            if (jsonEntity.contains("position"))
                setPosition(e, jsonEntity["position"]);

            if (jsonEntity.contains("name"))
            {
                std::string eName = jsonEntity["name"];
                setName(e, eName.c_str());
            }

            for (auto &[componentName, componentJson] : jsonEntity.at("components").items())
                componentUtils(componentName).setJsonComponentWithKeys(componentJson, e, entities);

            std::string applyTemplate = jsonEntity.at("template");
            if (!applyTemplate.empty())
                getTemplate(applyTemplate).createComponents(e, true);

        } catch (std::exception &exc)
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
    if (persistent.saveFinalPosition)
        j["position"] = getPosition(e);
    else if (persistent.saveSpawnPosition)
        j["position"] = persistent.spawnPosition;

    if (persistent.saveName)
        if (const char *eName = getName(e))
            j["name"] = eName;

    json &componentsJson = j["components"] = json::object();

    for (auto &componentTypeName : persistent.saveAllComponents ? ComponentUtils::getAllComponentTypeNames() : persistent.saveComponents)
    {
        auto utils = ComponentUtils::getFor(componentTypeName);
        if (utils->entityHasComponent(e, entities))
        {
            utils->getJsonComponentWithKeys(componentsJson[componentTypeName], e, entities);
        }
    }
}

void Room::tryToSaveRevivableEntity(entt::registry &, entt::entity entity)
{
    Persistent &p = entities.get<Persistent>(entity);
    if (!p.revive)
        return;

    revivableEntitiesToSave.push_back(json::object());
    persistentEntityToJson(entity, p, revivableEntitiesToSave.back());
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
