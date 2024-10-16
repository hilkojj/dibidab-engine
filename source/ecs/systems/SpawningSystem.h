#pragma once
#include "EntitySystem.h"
#include "../../level/room/Room.h"
#include "../components/Spawning.dibidab.h"

class SpawningSystem : public EntitySystem
{
    using EntitySystem::EntitySystem;

    EntityEngine *room = nullptr;

  protected:
    void update(double deltaTime, EntityEngine *room) override;
};
