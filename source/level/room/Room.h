#pragma once
#include "../../ecs/Engine.h"

#include <utils/delegate.h>
#include <json.hpp>

#include <set>

namespace dibidab::ecs
{
    struct Persistent;
}

namespace dibidab::level
{
    class Level;

    /**
     * A room is part of a level.
     */
    class Room : public ecs::Engine
    {
      public:

        Level &getLevel() const
        { return *level; };

        int getIndexInLevel() const
        { return roomI; };

        void update(double deltaTime) override;

        bool isLoadingPersistentEntities() const;

        void setPersistent(bool bPersistent);

        bool isPersistent() const;

        virtual void exportJsonData(json &);

        virtual void loadJsonData(const json &);

        virtual void exportBinaryData(std::vector<unsigned char> &dataOut)
        {};

        virtual void loadBinaryData(const unsigned char *data, uint64 dataLength)
        {};

        std::string name;

        delegate<void()> afterLoad;

      protected:

        virtual void preLoadInitialize();

        virtual void postLoadInitialize();

        std::list<ecs::System *> getSystemsToUpdate() const override;

        std::set<ecs::System *> systemsToUpdateDuringPause;

      private:

        void initialize(Level *lvl);

        void loadPersistentEntities();

        void persistentEntityToJson(entt::entity, const ecs::Persistent &, json &j) const;

        Level *level = nullptr;
        int roomI = -1;

        bool bIsPersistent = true;

        json jsonEntitiesToLoad;
        bool bLoadingPersistentEntities = false;

        friend void from_json(const json &j, Level &lvl);
        friend Level;
    };
}
