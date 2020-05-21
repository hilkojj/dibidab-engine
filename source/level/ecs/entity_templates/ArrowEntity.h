
#ifndef GAME_ARROWENTITY_H
#define GAME_ARROWENTITY_H

#include "EntityTemplate.h"
#include "../components/physics/Physics.h"
#include "../components/graphics/AsepriteView.h"
#include "../components/graphics/DrawPolyline.h"
#include "../components/combat/Arrow.h"
#include "../components/Polyline.h"

class ArrowEntity : public EntityTemplate
{
  public:
    entt::entity create() override
    {
        entt::entity e = room->entities.create();

        room->entities.assign<AABB>(e, ivec2(1));
        room->entities.assign<Physics>(e, 0.f, 0.f).ignorePlatforms = true;
        room->entities.assign<StaticCollider>(e);
        room->entities.assign<Arrow>(e);
        room->entities.assign<AsepriteView>(e, asset<aseprite::Sprite>("sprites/arrow"));

        room->entities.assign<Polyline>(e);
        room->entities.assign<DrawPolyline>(e, std::vector<uint8>{7u});

        return e;
    }
};


#endif
