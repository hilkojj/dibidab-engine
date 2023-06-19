
#ifndef GAME_BEHAVIORTREESYSTEM_H
#define GAME_BEHAVIORTREESYSTEM_H

#include "EntitySystem.h"

class BehaviorTreeSystem : public EntitySystem
{
    using EntitySystem::EntitySystem;

  protected:
    void init(EntityEngine *engine) override;

    void update(double deltaTime, EntityEngine *engine) override;
};


#endif //GAME_BEHAVIORTREESYSTEM_H
