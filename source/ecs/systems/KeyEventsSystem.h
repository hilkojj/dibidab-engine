
#ifndef GAME_KEYEVENTSSYSTEM_H
#define GAME_KEYEVENTSSYSTEM_H

#include "EntitySystem.h"

class KeyEventsSystem : public EntitySystem
{
    using EntitySystem::EntitySystem;

  protected:
    void init(EntityEngine *engine) override;

    void update(double deltaTime, EntityEngine *engine) override;

};


#endif
