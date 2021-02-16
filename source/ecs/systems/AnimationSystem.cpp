
#include <ecs/EntityEngine.h>
#include "AnimationSystem.h"

void AnimationSystem::update(double deltaTime, EntityEngine *engine)
{
    engine->entities.view<Animated>().each([&](Animated &an) {

        // all this copying is needed because the map can be modified while calling the animation functions
        auto mapCopy = an.animationUpdateFunctions;
        an.animationUpdateFunctions.clear();

        for (auto it = mapCopy.cbegin(); it != mapCopy.cend();)
        {
            if (it->second(deltaTime))
                mapCopy.erase(it++);
            else ++it;
        }

        for (auto it = an.animationUpdateFunctions.cbegin(); it != an.animationUpdateFunctions.cend(); it++)
            mapCopy[it->first] = it->second;

        an.animationUpdateFunctions = mapCopy;
    });
}

void AnimationSystem::init(EntityEngine *engine)
{
    engine->luaEnvironment["removeAnimation"] = [engine] (entt::entity e, const char *fieldName) -> bool {
        Animated *animated = engine->entities.try_get<Animated>(e);
        if (!animated)
            return false;
        return !!animated->animationUpdateFunctions.erase(fieldName);
    };
}
