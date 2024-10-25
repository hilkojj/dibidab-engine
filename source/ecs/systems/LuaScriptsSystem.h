#pragma once
#include "System.h"

#include "../components/LuaScripted.dibidab.h"

#include <entt/entity/fwd.hpp>

namespace dibidab::ecs
{
    class LuaScriptsSystem : public System
    {
        using System::System;

        Engine *engine;

      protected:
        void init(Engine *) override;

        void update(double deltaTime, Engine *room) override;

        void onDestroyed(entt::registry &, entt::entity);

        ~LuaScriptsSystem() override;
    };
}
