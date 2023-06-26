
#ifndef GAME_ROOM_H
#define GAME_ROOM_H

#include "../../ecs/EntityEngine.h"
#include "../../generated/Saving.hpp"
#include <json.hpp>

class Level;

/**
 * A room is part of a level.
 */
class Room : public EntityEngine
{
    Level *level = NULL;
    int roomI = -1;

    friend void from_json(const json& j, Level& lvl);
    friend Level;

    json persistentEntitiesToLoad, revivableEntitiesToSave;
    bool bLoadingPersistentEntities = false;

    void loadPersistentEntities();

  protected:
    virtual void initialize(Level *lvl);

  public:

    std::string name;

    delegate<void()> afterLoad;

    Level &getLevel() const { return *level; };

    int getIndexInLevel() const { return roomI; };

    int nrOfPersistentEntities() const;

    void update(double deltaTime) override;

    bool isLoadingPersistentEntities() const;

    virtual void toJson(json &);

    virtual void fromJson(const json &);

  private:

    void persistentEntityToJson(entt::entity, const Persistent &, json &j) const;

    void tryToSaveRevivableEntity(entt::registry &, entt::entity);
};

#endif
