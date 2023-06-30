
#include "PersistentEntityRef.h"

#include "../generated/Saving.hpp"

PersistentEntityRef::PersistentEntityRef() :
    resolved(true),
    entity(entt::null),
    persistentEntityId(0)
{

}

void PersistentEntityRef::set(entt::entity e, const entt::registry &reg)
{
    if (reg.valid(e))
    {
        if (const Persistent *persistent = reg.try_get<Persistent>(e))
        {
            persistentEntityId = persistent->persistentId;
        }
        else
        {
            throw gu_err("Tried to set a PersistentEntityRef to an non-persistent entity (entity: " + std::to_string(int(e)) + ")");
        }
    }
    else
    {
        persistentEntityId = 0;
    }
    resolved = true;
    entity = e;
}

entt::entity PersistentEntityRef::resolve(const entt::registry &reg) const
{
    if (resolved || tryResolve(reg, entity))
    {
        return entity;
    }
    throw gu_err("Failed to resolve a PersistentEntityRef (id = " + std::to_string(persistentEntityId) + ").\nIs the level still loading?");
}

bool PersistentEntityRef::tryResolve(const entt::registry &reg, entt::entity &outEntity) const
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

void to_json(json &j, const PersistentEntityRef &v)
{
    j = v.persistentEntityId;
}

void from_json(const json &j, PersistentEntityRef &v)
{
    v.persistentEntityId = j;
    v.resolved = false;
}
