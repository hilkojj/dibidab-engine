#pragma once
#include "EventEmitter.h"
#include "entity_templates/EntityTemplate.h"

#include "../luau.h"
#include "../macro_magic/component.h"

#include "../../external/entt/src/entt/entity/registry.hpp"

#include <utils/type_name.h>
#include <math/math_utils.h>

#include <map>
#include <list>

class EntitySystem;
class TimeOutSystem;

class EntityEngine
{
    bool bInitialized = false, bUpdating = false, bDestructing = false;

    TimeOutSystem *timeOutSystem;

  protected:

    std::list<EntitySystem *> systems;
    std::map<int, EntityTemplate *> entityTemplates;
    std::vector<std::string> entityTemplateNames;
    std::string templateFolder = "scripts/entities/";

    virtual void initializeLuaEnvironment();

  public:
    constexpr static const char *LUA_ENV_PTR_NAME = "enginePtr";

    sol::environment luaEnvironment;
    entt::registry entities;
    EventEmitter events;

    ivec2 cursorPosition = ivec2(0);

    void initialize();

    void addSystem(EntitySystem *sys, bool pushFront=false);

    std::list<EntitySystem *> getSystems() { return systems; }

    template <class EntitySystem_>
    EntitySystem_ *tryFindSystem()
    {
        auto it = std::find_if(
            std::begin(systems),
            std::end(systems),
            [](const auto sys) {
                return !!dynamic_cast<const EntitySystem_ *>(sys);
            }
        );
        if (it == systems.end())
            return nullptr;
        return (EntitySystem_ *) *it;
    }

    TimeOutSystem *getTimeOuts();

    template <class EntityTemplate_>
    EntityTemplate &getTemplate()
    {
        return getTemplate(typename_utils::getTypeName<EntityTemplate_>());
    }

    EntityTemplate &getTemplate(std::string name);

    EntityTemplate &getTemplate(int templateHash);

    const std::vector<std::string> &getTemplateNames() const;

    entt::entity getChildByName(entt::entity parent, const char *childName);

    template<typename Component>
    Component &getChildComponentByName(entt::entity parent, const char *childName)
    {
        entt::entity child = getChildByName(parent, childName);
        return entities.get<Component>(child);
    }

    template<typename Component>
    Component *tryGetChildComponentByName(entt::entity parent, const char *childName)
    {
        entt::entity child = getChildByName(parent, childName);
        return entities.try_get<Component>(child);
    }

    void luaTableToComponent(entt::entity, const std::string &componentName, const sol::table &);

    static const ComponentUtils &componentUtils(const std::string &componentName);

    entt::entity createChild(entt::entity parent, const char *childName="");

    void setParent(entt::entity child, entt::entity parent, const char *childName="");

    // returns false if name is already in use
    bool setName(entt::entity, const char *name=nullptr);

    entt::entity getByName(const char *name) const;

    const char *getName(entt::entity) const;

    const std::unordered_map<std::string, entt::entity> &getNamedEntities() const { return namedEntities; };

    template<typename type>
    void emitEntityEvent(entt::entity e, const type &event, const char *customEventName=nullptr)
    {
        if (auto *emitter = entities.try_get<EventEmitter>(e))
            emitter->emit(event, customEventName);
    }

    virtual ~EntityEngine();

    bool isDestructing() const;

    virtual void update(double deltaTime);

    bool isUpdating() const;

    virtual vec3 getPosition(entt::entity) const;

    virtual void setPosition(entt::entity, const vec3 &);

  protected:

    virtual std::list<EntitySystem *> getSystemsToUpdate() const;

    template <class EntityTemplate>
    void registerEntityTemplate()
    {
        auto name = typename_utils::getTypeName<EntityTemplate>();
        addEntityTemplate(name, new EntityTemplate());
    }

    void registerLuaEntityTemplate(const char *assetPath);

    void addEntityTemplate(const std::string &name, EntityTemplate *);

  private:
    void onChildCreation(entt::registry &, entt::entity);

    void onChildDeletion(entt::registry &, entt::entity);

    void onParentDeletion(entt::registry &, entt::entity);

    std::unordered_map<std::string, entt::entity> namedEntities;

    void onEntityDenaming(entt::registry &, entt::entity);

};
