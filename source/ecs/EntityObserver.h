#pragma once
#include <entt/entity/registry.hpp>

#include <list>

class EntityObserver
{
    struct Callback
    {
        std::function<void()> function;
        bool bUnregistered = false;
    };

    template<class Component>
    struct Observed
    {
        std::list<std::shared_ptr<Callback>> onConstructCallbacks;
        std::list<std::shared_ptr<Callback>> onDestroyCallbacks;
    };

  public:

    struct Handle
    {
        entt::entity getEntity() const;

      private:
        friend EntityObserver;

        Handle(const std::shared_ptr<Callback> &callback, bool bOnConstruct, entt::entity e);

        const std::shared_ptr<Callback> callback;
        const bool bOnConstruct;
        const entt::entity entity;
    };

    template<class Component>
    EntityObserver(std::in_place_type_t<Component>, entt::registry &registry) :
        registry(registry)
    {
        registry.on_construct<Component>().template connect<&EntityObserver::onComponentConstructed>(this);
        registry.on_destroy<Component>().template connect<&EntityObserver::onComponentDestroyed>(this);

        tryGetOnConstructCallbacks = [] (entt::registry &registry, entt::entity e) -> std::list<std::shared_ptr<Callback>> *
        {
            if (Observed<Component> *observed = registry.try_get<Observed<Component>>(e))
            {
                return &observed->onConstructCallbacks;
            }
            return nullptr;
        };
        tryGetOnDestroyCallbacks = [] (entt::registry &registry, entt::entity e) -> std::list<std::shared_ptr<Callback>> *
        {
            if (Observed<Component> *observed = registry.try_get<Observed<Component>>(e))
            {
                return &observed->onDestroyCallbacks;
            }
            return nullptr;
        };

        getOrCreateOnConstructCallbacks = [] (entt::registry &registry, entt::entity e)
        {
            return &registry.get_or_assign<Observed<Component>>(e).onConstructCallbacks;
        };
        getOrCreateOnDestroyCallbacks = [] (entt::registry &registry, entt::entity e)
        {
            return &registry.get_or_assign<Observed<Component>>(e).onDestroyCallbacks;
        };
    }

    // WARNING: The callback is not allowed to destroy the component within its body
    Handle onConstruct(entt::entity e, std::function<void()> callback);

    Handle onDestroy(entt::entity e, std::function<void()> callback);

    void unregister(Handle &handle);

  private:

    void onComponentConstructed(entt::registry &registry, entt::entity e);

    void onComponentDestroyed(entt::registry &registry, entt::entity e);

    void callCallbacks(std::list<std::shared_ptr<Callback>> &callbacks, entt::entity e);

    entt::registry &registry;

    std::function<std::list<std::shared_ptr<Callback>> *(entt::registry &, entt::entity)> tryGetOnConstructCallbacks;
    std::function<std::list<std::shared_ptr<Callback>> *(entt::registry &, entt::entity)> tryGetOnDestroyCallbacks;

    std::function<std::list<std::shared_ptr<Callback>> *(entt::registry &, entt::entity)> getOrCreateOnConstructCallbacks;
    std::function<std::list<std::shared_ptr<Callback>> *(entt::registry &, entt::entity)> getOrCreateOnDestroyCallbacks;
};
