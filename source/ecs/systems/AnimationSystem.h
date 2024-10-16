#pragma once
#include "EntitySystem.h"
#include "../components/Animation.h"

class AnimationSystem : public EntitySystem
{
    using EntitySystem::EntitySystem;

  protected:
    void init(EntityEngine *engine) override;

    void update(double deltaTime, EntityEngine *engine) override;

};
