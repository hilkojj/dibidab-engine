#pragma once
#include "System.h"

namespace dibidab::ecs
{
    class SpawningSystem : public System
    {
        using System::System;

      protected:
        void update(double deltaTime, Engine *engine) override;
    };
}
