
#ifndef GAME_ARROWSYSTEM_H
#define GAME_ARROWSYSTEM_H

#include "../EntitySystem.h"
#include "../../../room/Room.h"
#include "../../components/combat/Arrow.h"
#include "../../components/physics/Physics.h"
#include "../../components/graphics/AsepriteView.h"
#include "../../components/combat/Aiming.h"
#include "../../components/DespawnAfter.h"
#include "../../components/combat/Health.h"
#include "../../components/combat/KnockBack.h"

class ArrowSystem : public EntitySystem
{
    using EntitySystem::EntitySystem;

  protected:
    void update(double deltaTime, Room *room) override
    {
        room->entities.view<Arrow, AABB, Physics, AsepriteView>().each([&](
            auto e, Arrow &arrow, AABB &aabb, Physics &physics, AsepriteView &sprite
        ){
            if (physics.touches.anything) // Terrain
            {
                room->entities.remove<Physics>(e);
                room->entities.assign<DespawnAfter>(e, 100);
                return;
            }

            room->entities.view<AABB, Health, AsepriteView>().each([&](
                auto eOther, AABB &aabbOther, Health &healthOther, AsepriteView &spriteOther
            ){
                if (!aabb.overlaps(aabbOther))
                    return;

                healthOther.curHealth -= 5;
                room->entities.destroy(e);

                KnockBack *knockBack = room->entities.try_get<KnockBack>(eOther);
                if (knockBack != nullptr) {
                    knockBack->knockBackForce = 10.0f;
                    knockBack->knockBackDirection = normalize(vec2(min(aabbOther.center, aabb.center)));
                }

                if (healthOther.curHealth <= 0.0f) {
                    room->entities.assign<DespawnAfter>(eOther, 0.0f);
                }
            });

            // choose sprite frame based on velocity:
            vec2 dir = normalize(physics.velocity);
            float angle = atan2(dir.y, dir.x) + mu::PI;

            int frame = round(angle / (2 * mu::PI) * sprite.sprite->frameCount);
            frame = max(0, frame);
            if (frame == sprite.sprite->frameCount)
                frame = 0;
            sprite.frame = frame;
        });
    }

};


#endif
