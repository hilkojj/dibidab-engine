
#ifndef GAME_ROOM_H
#define GAME_ROOM_H

#include "../../ecs/EntityEngine.h"

#include <utils/delegate.h>
#include <json.hpp>

#include <set>

class Level;
struct Persistent;

/**
 * A room is part of a level.
 */
class Room : public EntityEngine
{
  public:

    Level &getLevel() const { return *level; };

    int getIndexInLevel() const { return roomI; };

    void update(double deltaTime) override;

    bool isLoadingPersistentEntities() const;

    int getNumPersistentEntities() const;

    void setPersistent(bool bPersistent);

    bool isPersistent() const;

    virtual void exportJsonData(json &);

    virtual void loadJsonData(const json &);

    virtual void exportBinaryData(std::vector<unsigned char> &dataOut) {};

    virtual void loadBinaryData(const unsigned char *data, uint64 dataLength) {};

    std::string name;

    delegate<void()> afterLoad;

  protected:

    virtual void preLoadInitialize();

    virtual void postLoadInitialize();

    std::list<EntitySystem *> getSystemsToUpdate() const override;

    std::set<EntitySystem *> systemsToUpdateDuringPause;

  private:

    void initialize(Level *lvl);

    void loadPersistentEntities();

    void persistentEntityToJson(entt::entity, const Persistent &, json &j) const;

    void tryToSaveRevivableEntity(entt::registry &, entt::entity);

    Level *level = nullptr;
    int roomI = -1;

    bool bIsPersistent = true;

    json persistentEntitiesToLoad, revivableEntitiesToSave;
    bool bLoadingPersistentEntities = false;

    friend void from_json(const json &j, Level &lvl);
    friend Level;
};

#endif
