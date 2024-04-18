
#ifndef GAME_PERSISTENTENTITYREF_H
#define GAME_PERSISTENTENTITYREF_H

#include "../../external/entt/src/entt/entity/registry.hpp"

#include <utils/math_utils.h>

#include <json.hpp>

#include <map>

typedef uint64 PersistentEntityID;

struct PersistentEntities
{
    PersistentEntityID idCounter;
    std::map<PersistentEntityID, entt::entity> persistentEntityIdMap;
};

struct PersistentEntityRef
{
    PersistentEntityRef();

    void set(entt::entity, const entt::registry &);

    entt::entity resolve(const entt::registry &) const;

    bool tryResolve(const entt::registry &, entt::entity &) const;

    PersistentEntityID getId() const;

    bool operator==(const PersistentEntityRef &other) const;

    bool operator<(const PersistentEntityRef &other) const;

  private:
    mutable bool resolved;
    mutable entt::entity entity;
    PersistentEntityID persistentEntityId;

    friend void to_json(json &j, const PersistentEntityRef &v);

    friend void from_json(const json &j, PersistentEntityRef &v);

    friend std::hash<PersistentEntityRef>;
};

void to_json(json &j, const PersistentEntityRef &v);

void from_json(const json &j, PersistentEntityRef &v);

namespace std
{
    template<>
    struct hash<PersistentEntityRef>
    {
        inline size_t operator()(const PersistentEntityRef &ref) const
        {
            return ref.persistentEntityId;
        }
    };
}

#endif //GAME_PERSISTENTENTITYREF_H
