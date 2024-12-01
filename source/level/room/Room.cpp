#include "Room.h"

#include "../Level.h"

#include "../../ecs/systems/SpawningSystem.h"
#include "../../ecs/systems/LuaScriptsSystem.h"
#include "../../ecs/components/Persistent.dibidab.h"
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

    std::map<entt::entity, entt::entity> hintToEntity;

    for (const json &jsonEntity : jsonEntitiesToLoad)
    {
        const entt::entity hint = jsonEntity.at("entityHint");
        if (hint != entt::null)
        {
            if (hintToEntity.find(hint) != hintToEntity.end())
            {
                std::cerr << "Desired entity identifier #" << std::to_string(int(hint)) << " was requested before" << std::endl;
                continue;
            }

            const entt::entity entity = entities.create(hint);
            hintToEntity[hint] = entity;

            if (hint != entt::null && hint != entity)
            {
                std::cerr << "Could not create desired entity identifier #" << std::to_string(int(hint)) << std::endl;
            }
        }
    }

    for (const json &jsonEntity : jsonEntitiesToLoad)
    {
        const entt::entity hint = jsonEntity.at("entityHint");
        entt::entity entity = entt::null;

        if (hint != entt::null)
        {
            auto hintIt = hintToEntity.find(hint);
            if (hintIt != hintToEntity.end())
            {
                entity = hintIt->second;
                hintToEntity.erase(hintIt);
            }
        }

        if (!entities.valid(entity))
        {
            entity = entities.create();
        }

        try
        {
            auto &p = entities.assign<ecs::Persistent>(entity);
            p.entityHint = hint;
            p.data = jsonEntity.at("data");

            if (jsonEntity.contains("name"))
            {
                std::string eName = jsonEntity["name"];
                setName(entity, eName.c_str());
            }

            for (auto &[componentName, componentJson] : jsonEntity.at("components").items())
            {
                if (const ComponentInfo *info = findComponentInfo(componentName.c_str()))
                {
                    if (info->setFromJson)
                    {
                        info->setFromJson(componentJson, entity, entities);
                    }
                    else
                    {
                        info->addComponent(entity, entities);
                    }
                }
                else
                {
                    std::cerr << "Encountered non existing component '" << componentName << "' while loading entity:\n"
                        << jsonEntity.dump() << std::endl;
                }
            }

            const std::string applyTemplate = jsonEntity.at("template");
            if (!applyTemplate.empty())
            {
                getTemplate(applyTemplate).createComponents(entity, true);
            }
        }
        catch (std::exception &exc)
        {
            std::cerr << "Error while loading entity from JSON: \n" << exc.what() << std::endl;
            std::cerr << "entity json: " << jsonEntity.dump() << std::endl;
            entities.destroy(entity);
        }
    }
    jsonEntitiesToLoad.clear();
    bLoadingPersistentEntities = false;
}

void dibidab::level::Room::persistentEntityToJson(entt::entity e, const ecs::Persistent &persistent, json &j) const
{
    j["entityHint"] = persistent.entityHint;
    j["template"] = persistent.applyTemplateOnLoad;
    j["data"] = persistent.data;

    if (persistent.bSaveName)
        if (const char *eName = getName(e))
            j["name"] = eName;

    json &componentsJson = j["components"] = json::object();

    for (auto &componentTypeName : persistent.saveComponents)
    {
        if (const ComponentInfo *info = findComponentInfo(componentTypeName.c_str()))
        {
            if (info->hasComponent(e, entities))
            {
                if (info->getJsonObject)
                {
                    info->getJsonObject(e, entities, componentsJson[componentTypeName]);
                }
                else
                {
                    componentsJson[componentTypeName] = json::object();
                }
            }
        }
    }
}

void dibidab::level::Room::exportJsonData(json &j)
{
    events.emit(0, "BeforeSave");
    j = json {
        {"name", name},
        {"entities", json::array()},
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
    jsonEntitiesToLoad = j.at("entities");
}
