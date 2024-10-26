#pragma once
#include "EventEmitter.h"

#include <utils/type_name.h>
#include <math/math_utils.h>

#include <entt/entity/registry.hpp>
#include <sol/sol.hpp>

#include <map>
#include <list>

namespace dibidab
{
    struct ComponentInfo;
}

namespace dibidab::ecs
{
    class System;
    class Template;
    class Observer;
    class TimeOutSystem;

    class Engine
    {
      public:
        void initialize();

        void addSystem(System *sys, bool pushFront = false);

        std::list<System *> getSystems();

        template<class SystemType>
        SystemType *tryFindSystem()
        {
            for (System *system : systems)
            {
                if (dynamic_cast<SystemType *>(system) != nullptr)
                {
                    return (SystemType *) system;
                }
            }
            return nullptr;
        }

        TimeOutSystem *getTimeOuts();

        const std::string &getTemplateDirectoryPath() const;

        Template &getTemplate(std::string name);

        Template &getTemplate(int templateHash);

        const std::vector<std::string> &getTemplateNames() const;

        entt::entity getChildByName(entt::entity parent, const char *childName);

        entt::entity createChild(entt::entity parent, const char *childName = "");

        void setParent(entt::entity child, entt::entity parent, const char *childName = "");

        // returns false if name is already in use
        bool setName(entt::entity, const char *name = nullptr);

        entt::entity getByName(const char *name) const;

        const char *getName(entt::entity) const;

        const std::unordered_map<std::string, entt::entity> &getNamedEntities() const;;

        Observer &getObserverForComponent(const ComponentInfo &component);

        template<typename EventType>
        void emitEntityEvent(entt::entity e, const EventType &event, const char *customEventName = nullptr)
        {
            if (auto *emitter = entities.try_get<EventEmitter>(e))
            {
                emitter->emit(event, customEventName);
            }
        }

        virtual ~Engine();

        bool isDestructing() const;

        virtual void update(double deltaTime);

        bool isUpdating() const;

        constexpr static const char *LUA_ENV_PTR_NAME = "enginePtr";

        sol::environment luaEnvironment;
        entt::registry entities;
        EventEmitter events;
        ivec2 cursorPosition = ivec2(0);

      protected:

        virtual void initializeLuaEnvironment();

        virtual std::list<System *> getSystemsToUpdate() const;

        template<class EntityTemplate>
        void registerEntityTemplate()
        {
            auto name = typename_utils::getTypeName<EntityTemplate>();
            addEntityTemplate(name, new EntityTemplate());
        }

        void registerLuaEntityTemplate(const char *assetPath);

        void addEntityTemplate(const std::string &name, Template *);

        std::list<System *> systems;
        std::map<int, Template *> entityTemplates;
        std::vector<std::string> entityTemplateNames;
        std::string templateDirectoryPath = "scripts/entities/";

      private:
        void onChildCreation(entt::registry &, entt::entity);

        void onChildDeletion(entt::registry &, entt::entity);

        void onParentDeletion(entt::registry &, entt::entity);

        std::unordered_map<std::string, entt::entity> namedEntities;

        void onEntityDenaming(entt::registry &, entt::entity);

        void setComponentFromLua(entt::entity entity, const sol::table &component);

        bool bInitialized = false;
        bool bUpdating = false;
        bool bDestructing = false;
        TimeOutSystem *timeOutSystem;
        std::map<const ComponentInfo *, Observer *> observerPerComponent;
    };
}
