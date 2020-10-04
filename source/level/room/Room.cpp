
#include <gu/profiler.h>
#include "Room.h"

#include "../../ecs/systems/PlayerControlSystem.h"
#include "../../ecs/systems/AudioSystem.h"
#include "../../ecs/systems/SpawningSystem.h"
#include "../../ecs/systems/LuaScriptsSystem.h"

void Room::initialize(Level *lvl)
{
    assert(lvl != NULL);

    level = lvl;

    entities.on_destroy<Persistent>().connect<&Room::tryToSaveRevivableEntity>(this);

    addSystem(new PlayerControlSystem("player control"));
    addSystem(new AudioSystem("audio"));


    addSystem(new LuaScriptsSystem("lua functions"), true); // execute lua functions first, in case they might spawn entities, same reason as below:
    addSystem(new SpawningSystem("(de)spawning"), true); // SPAWN ENTITIES FIRST, so they get a chance to be updated before being rendered
    EntityEngine::initialize();

    loadPersistentEntities();
}

void Room::update(double deltaTime)
{
    gu::profiler::Zone roomZone("room " + std::to_string(getIndexInLevel()));

    EntityEngine::update(deltaTime);
}

void Room::loadPersistentEntities()
{
    for (json &jsonEntity : persistentEntitiesToLoad)
    {
        auto e = entities.create();
        assert(entities.valid(e));
        try
        {
            auto &p = entities.assign<Persistent>(e);
            p.data = jsonEntity.at("data");
            if (jsonEntity.contains("position"))
                setPosition(e, jsonEntity["position"]);

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
}

int Room::nrOfPersistentEntities() const
{
    return persistentEntitiesToLoad.is_array() ? persistentEntitiesToLoad.size() : 0;
}

void Room::persistentEntityToJson(entt::entity e, const Persistent &persistent, json &j) const
{
    j["template"] = persistent.applyTemplateOnLoad;
    j["data"] = persistent.data;
    if (persistent.saveFinalPosition)
        j["position"] = getPosition(e);
    else if (persistent.saveSpawnPosition)
        j["position"] = persistent.spawnPosition;

    json &componentsJson = j["components"] = json::object();

    for (auto &componentTypeName : persistent.saveAllComponents ? ComponentUtils::getAllComponentTypeNames() : persistent.saveComponents)
    {
        auto utils = ComponentUtils::getFor(componentTypeName);
        if (utils->entityHasComponent(e, entities))
            utils->getJsonComponentWithKeys(componentsJson[componentTypeName], e, entities);
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

void Room::toJson(json &j) const
{
    j = json{
        {"name", name},
        {"entities", revivableEntitiesToSave}
    };
    entities.view<const Persistent>().each([&](auto e, const Persistent &persistent) {

        j["entities"].push_back(json::object());
        persistentEntityToJson(e, persistent, j["entities"].back());
    });
}

void Room::fromJson(const json &j)
{
    name = j.at("name");
    persistentEntitiesToLoad = j.at("entities");
}
