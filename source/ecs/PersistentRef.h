#pragma once
#include <math/math_utils.h>
#include <json.hpp>

#include <entt/entity/fwd.hpp>

#include <map>

namespace dibidab::ecs
{
    typedef uint64 PersistentID;

    struct PersistentEntities
    {
        PersistentID idCounter;
        std::map<PersistentID, entt::entity> persistentEntityIdMap;
    };

    struct PersistentRef
    {
        PersistentRef();

        void set(entt::entity, const entt::registry &);

        entt::entity resolve(const entt::registry &) const;

        bool tryResolve(const entt::registry &, entt::entity &) const;

        PersistentID getId() const;

        bool operator==(const PersistentRef &other) const;

        bool operator<(const PersistentRef &other) const;

      private:
        mutable bool resolved;
        mutable entt::entity entity;
        PersistentID persistentEntityId;

        friend void to_json(json &j, const PersistentRef &v);

        friend void from_json(const json &j, PersistentRef &v);

        friend std::hash<PersistentRef>;
    };

    void to_json(json &j, const PersistentRef &v);

    void from_json(const json &j, PersistentRef &v);
}

namespace std
{
    template<>
    struct hash<dibidab::ecs::PersistentRef>
    {
        inline size_t operator()(const dibidab::ecs::PersistentRef &ref) const
        {
            return ref.persistentEntityId;
        }
    };
}
