
#ifndef GAME_COMPONENT_H
#define GAME_COMPONENT_H

#include "../../external/entt/src/entt/entity/registry.hpp"
#include "../ecs/PersistentEntityRef.h"
#include "../ecs/EntityObserver.h"
#include "serializable.h"
#include <ecs/components/Animation.h>
#include <utils/hashing.h>

struct ComponentUtils
{
    template<class Component>
    struct TemplatedEntityObserverWrapper
    {
        EntityObserver observer;

        TemplatedEntityObserverWrapper(entt::registry &registry) :
            observer(std::in_place_type<Component>, registry)
        {
        }
    };

  public:

    // todo: improve naming of these functions:

    std::function<bool(entt::entity, const entt::registry &)> entityHasComponent;
    std::function<void(json &, entt::entity, const entt::registry &)> getJsonComponent, getJsonComponentWithKeys;
    std::function<void(const json &, entt::entity, entt::registry &)> setJsonComponent, setJsonComponentWithKeys;
    std::function<void(entt::entity, entt::registry &)> removeComponent;
    std::function<json()> getDefaultJsonComponent;

    std::function<void(const sol::table &, entt::entity, entt::registry &)> setFromLuaTable;

    std::function<void(sol::table &, entt::registry &)> registerLuaFunctions;

    std::function<EntityObserver *(entt::registry &)> getEntityObserver;

    const SerializableStructInfo *structInfo = nullptr;

    template <class Component>
    const static ComponentUtils *create()
    {
        if (!utils) utils = new std::map<std::string, ComponentUtils *>();
        if (!utilsByType) utilsByType = new std::map<std::size_t, ComponentUtils *>();
        if (!names) names = new std::vector<std::string>();

        // for some reason ComponentUtils::create<Component>() is called multiple times for the same Component on Windows.... wtf...
        if (getFor(Component::COMPONENT_NAME))
            return getFor(Component::COMPONENT_NAME);

        ComponentUtils *u = new ComponentUtils();
        u->structInfo = &Component::STRUCT_INFO;

        (*utils)[Component::COMPONENT_NAME] = u;
        (*utilsByType)[typeid(Component).hash_code()] = u;
        names->push_back(Component::COMPONENT_NAME);

        u->entityHasComponent = [] (entt::entity e, const entt::registry &reg)
        {
            return reg.valid(e) && reg.has<Component>(e);    // todo: why the valid() check?
        };
        u->getJsonComponent = [] (json &j, entt::entity e, const entt::registry &reg)
        {
            reg.get<Component>(e).toJsonArray(j);
        };
        u->getJsonComponentWithKeys = [] (json &j, entt::entity e, const entt::registry &reg)
        {
            reg.get<Component>(e).toJson(j);
        };
        u->setJsonComponent = [] (const json &j, entt::entity e, entt::registry &reg)
        {
            reg.get_or_assign<Component>(e).fromJsonArray(j);
        };
        u->setJsonComponentWithKeys = [] (const json &j, entt::entity e, entt::registry &reg)
        {
            reg.get_or_assign<Component>(e).fromJson(j);
        };
        u->removeComponent = [] (entt::entity e, entt::registry &reg)
        {
            reg.remove_if_exists<Component>(e);
        };
        u->getDefaultJsonComponent = [] { return Component(); };

        u->setFromLuaTable = [] (const sol::table &table, entt::entity e, entt::registry &reg)
        {
            auto optional = table.as<sol::optional<Component &>>();
            if (optional.has_value())
            {
                if (reg.has<Component>(e))
                    reg.get<Component>(e).copyFieldsFrom(optional.value());
                else
                    reg.assign<Component>(e, optional.value());
            }

            else // TODO: give error instead?
                reg.get_or_assign<Component>(e).fromLuaTable(table);
        };

        u->registerLuaFunctions = [u] (sol::table &table, entt::registry &reg)
        {
            sol::table componentUtilsTable = table[Component::COMPONENT_NAME].template get_or_create<sol::table>();
            Component::registerEntityEngineFunctions(componentUtilsTable, reg);
            componentUtilsTable["componentUtils"] = u;
        };

        u->getEntityObserver = [] (entt::registry &reg)
        {
            if (TemplatedEntityObserverWrapper<Component> *wrapper = reg.try_ctx<TemplatedEntityObserverWrapper<Component>>())
            {
                return &wrapper->observer;
            }
            TemplatedEntityObserverWrapper<Component> &wrapper = reg.ctx_or_set<TemplatedEntityObserverWrapper<Component>>(reg);
            return &wrapper.observer;
        };

        return u;
    }

    template<class Component>
    static const ComponentUtils *getFor()
    {
        return utilsByType->operator[](typeid(Component).hash_code());
    }

    static const ComponentUtils *getFor(const std::string &componentName)
    {
        return utils->operator[](componentName);
    }

    static const ComponentUtils *getFromLuaComponentTable(const sol::table &componentTable)
    {
        ComponentUtils *ptr = componentTable.get<ComponentUtils *>("componentUtils");
        if (!ptr)
        {
            throw gu_err("Provided table is not a component table!");
        }
        return ptr;
    }

    static const std::vector<std::string> &getAllComponentTypeNames()
    {
        return *names;
    }

  private:
    static std::map<std::size_t, ComponentUtils *> *utilsByType;
    static std::map<std::string, ComponentUtils *> *utils;
    static std::vector<std::string> *names;

};

// only used to mark fields "read-only" in the in-game inspector, TODO: remove this unnecessary hack?
template <typename Type>
using final = Type;

#endif //GAME_COMPONENT_H
