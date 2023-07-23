
#ifndef GAME_TIMEOUTSYSTEM_H
#define GAME_TIMEOUTSYSTEM_H

#include "EntitySystem.h"
#include "../EntityEngine.h"

#include <utils/delegate.h>


class TimeOutSystem : public EntitySystem
{
    using EntitySystem::EntitySystem;

  public:
    delegate_method callAfter(float seconds, entt::entity waitingEntity, const std::function<void()> &callback);

  protected:
    void init(EntityEngine *engine) override;

    void update(double deltaTime, EntityEngine *engine) override;

  private:
    EntityEngine *engine = nullptr;
};


#endif //GAME_TIMEOUTSYSTEM_H
