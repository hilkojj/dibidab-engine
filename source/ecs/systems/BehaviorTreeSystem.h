#pragma once
#include "System.h"

namespace dibidab::ecs
{
    class BehaviorTreeSystem : public System
    {
        using System::System;

      protected:
        void init(Engine *engine) override;

        void update(double deltaTime, Engine *engine) override;
    };
}
