#pragma once
#include "System.h"

namespace dibidab::ecs
{
    class KeyEventsSystem : public System
    {
        using System::System;

      protected:
        void init(Engine *engine) override;

        void update(double deltaTime, Engine *engine) override;

    };
}
