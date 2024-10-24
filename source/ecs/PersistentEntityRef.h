#pragma once
#include <math/math_utils.h>

#include <entt/entity/registry.hpp>
#include <json.hpp>

#include <map>

namespace dibidab::ecs
{
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
}

namespace std
{
    template<>
    struct hash<dibidab::ecs::PersistentEntityRef>
    {
        inline size_t operator()(const dibidab::ecs::PersistentEntityRef &ref) const
        {
            return ref.persistentEntityId;
        }
    };
}
