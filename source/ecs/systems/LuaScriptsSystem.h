#pragma once
#include "EntitySystem.h"
#include "../../level/room/Room.h"
#include "../components/LuaScripted.dibidab.h"

class LuaScriptsSystem : public EntitySystem
{
    using EntitySystem::EntitySystem;

    EntityEngine *engine;

  protected:
    void init(EntityEngine *) override;

    void update(double deltaTime, EntityEngine *room) override;

    void onDestroyed(entt::registry &, entt::entity);

    ~LuaScriptsSystem() override;

};
