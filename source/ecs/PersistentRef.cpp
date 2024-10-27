#include "PersistentRef.h"

#include "components/Persistent.dibidab.h"

#include <utils/gu_error.h>

#include <entt/entity/registry.hpp>

dibidab::ecs::PersistentRef::PersistentRef() :
    resolved(true),
    entity(entt::null),
    persistentEntityId(0)
{

}

void dibidab::ecs::PersistentRef::set(entt::entity e, const entt::registry &reg)
{
    if (reg.valid(e))
    {
        if (const Persistent *persistent = reg.try_get<Persistent>(e))
        {
            persistentEntityId = persistent->persistentId;
        }
        else
        {
            throw gu_err("Tried to set a dibidab::ecs::PersistentRef to an non-persistent entity (entity: " + std::to_string(int(e)) + ")");
        }
    }
    else
    {
        persistentEntityId = 0;
    }
    resolved = true;
    entity = e;
}

entt::entity dibidab::ecs::PersistentRef::resolve(const entt::registry &reg) const
{
    if (resolved || tryResolve(reg, entity))
    {
        return entity;
    }
    throw gu_err("Failed to resolve a dibidab::ecs::PersistentRef (id = " + std::to_string(persistentEntityId) + ").\nIs the level still loading?");
}

bool dibidab::ecs::PersistentRef::tryResolve(const entt::registry &reg, entt::entity &outEntity) const
{
    if (persistentEntityId == 0)
    {
        entity = entt::null;
        resolved = true;
        outEntity = entity;
        return true;
    }
    else if (const PersistentEntities *persistentEntities = reg.try_ctx<PersistentEntities>())
    {
        auto it = persistentEntities->persistentEntityIdMap.find(persistentEntityId);
        if (it != persistentEntities->persistentEntityIdMap.end())
        {
            entity = it->second;
            resolved = true;
            outEntity = entity;
            return true;
        }
    }
    outEntity = entt::null;
    return false;
}

dibidab::ecs::PersistentID dibidab::ecs::PersistentRef::getId() const
{
    return persistentEntityId;
}

bool dibidab::ecs::PersistentRef::operator==(const PersistentRef &other) const
{
    return persistentEntityId == other.persistentEntityId;
}

bool dibidab::ecs::PersistentRef::operator<(const PersistentRef &other) const
{
    return persistentEntityId < other.persistentEntityId;
}

void dibidab::ecs::to_json(json &j, const dibidab::ecs::PersistentRef &v)
{
    j = v.persistentEntityId;
}

void dibidab::ecs::from_json(const json &j, dibidab::ecs::PersistentRef &v)
{
    v.persistentEntityId = j;
    v.resolved = false;
}
