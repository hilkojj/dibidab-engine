
#ifndef GAME_ANIMATIONSYSTEM_H
#define GAME_ANIMATIONSYSTEM_H

#include "EntitySystem.h"
#include "../components/Animation.h"


class AnimationSystem : public EntitySystem
{
    using EntitySystem::EntitySystem;

  protected:
    void update(double deltaTime, EntityEngine *engine) override;

};


#endif
