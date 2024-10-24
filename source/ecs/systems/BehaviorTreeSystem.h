#pragma once
#include "EntitySystem.h"

namespace dibidab::ecs
{
    class BehaviorTreeSystem : public EntitySystem
    {
        using EntitySystem::EntitySystem;

      protected:
        void init(EntityEngine *engine) override;

        void update(double deltaTime, EntityEngine *engine) override;
    };
}
