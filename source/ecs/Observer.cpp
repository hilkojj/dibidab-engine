
#include "Observer.h"

#include <utils/gu_error.h>

entt::entity dibidab::ecs::Observer::Handle::getEntity() const
{
    return entity;
}

dibidab::ecs::Observer::Handle::Handle(
    const std::shared_ptr<Callback> &callback,
    bool bOnConstruct,
    entt::entity e
) :
    callback(callback),
    bOnConstruct(bOnConstruct),
    entity(e)
{
}

dibidab::ecs::Observer::Handle dibidab::ecs::Observer::onConstruct(entt::entity e, std::function<void()> callback)
{
    std::list<std::shared_ptr<Callback>> *callbacks = getOrCreateOnConstructCallbacks(registry, e);
    callbacks->push_back(std::make_shared<Callback>(Callback { std::move(callback) }));
    return {
        callbacks->back(),
        true,
        e
    };
}

dibidab::ecs::Observer::Handle dibidab::ecs::Observer::onDestroy(entt::entity e, std::function<void()> callback)
{
    std::list<std::shared_ptr<Callback>> *callbacks = getOrCreateOnDestroyCallbacks(registry, e);
    callbacks->push_back(std::make_shared<Callback>(Callback { std::move(callback) }));
    return {
        callbacks->back(),
        false,
        e
    };
}

void dibidab::ecs::Observer::unregister(Handle &handle)
{
    std::list<std::shared_ptr<Callback>> *callbacks = (handle.bOnConstruct ? tryGetOnConstructCallbacks : tryGetOnDestroyCallbacks)(
        registry, handle.entity
    );
    if (callbacks)
    {
        callbacks->remove_if([&] (const std::shared_ptr<Callback> &callback)
        {
            return callback.get() == handle.callback.get();
        });
        handle.callback->bUnregistered = true;
    }
}

void dibidab::ecs::Observer::onComponentConstructed(entt::registry &, entt::entity entity)
{
    if (std::list<std::shared_ptr<Callback>> *callbacks = tryGetOnConstructCallbacks(registry, entity))
    {
        callCallbacks(*callbacks, entity);
    }
}

void dibidab::ecs::Observer::onComponentDestroyed(entt::registry &, entt::entity entity)
{
    if (std::list<std::shared_ptr<Callback>> *callbacks = tryGetOnDestroyCallbacks(registry, entity))
    {
        callCallbacks(*callbacks, entity);
    }
}

void dibidab::ecs::Observer::callCallbacks(std::list<std::shared_ptr<Callback>> &callbacks, entt::entity entity)
{
    std::list<std::shared_ptr<Callback>> callbacksCopy = callbacks;
    for (std::shared_ptr<Callback> &callback : callbacksCopy)
    {
        if (!callback->bUnregistered)
        {
            callback->function();
        }
        if (entt::registry::version(entity) != registry.current(entity))
        {
            throw gu_err("Do not destroy an entity during a callback of the EntityObserver!");
        }
    }
}
