
#ifndef GAME_AUDIOSYSTEM_H
#define GAME_AUDIOSYSTEM_H

#include "EntitySystem.h"
#include "../../level/room/Room.h"
#include "../../game/dibidab.h"
#include "../../generated/SoundSpeaker.hpp"

class AudioSystem : public EntitySystem
{
    using EntitySystem::EntitySystem;

    delegate_method onPlayerLeft;

  protected:

    void init(EntityEngine *engine) override;

    void update(double deltaTime, EntityEngine *room) override;

};


#endif
