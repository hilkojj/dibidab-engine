
#include <ecs/EntityEngine.h>
#include "AnimationSystem.h"

void AnimationSystem::update(double deltaTime, EntityEngine *engine)
{
    engine->entities.view<Animated>().each([&](auto e, Animated &an) {

        // all this copying is needed because the map can be modified while calling the animation functions
        auto mapCopy = an.animationUpdateFunctions;
        an.animationUpdateFunctions.clear();

        for (auto it = mapCopy.cbegin(); it != mapCopy.cend();)
        {
            if (it->second(deltaTime))
                mapCopy.erase(it++);
            else ++it;
        }

        // REFERENCE TO an MIGHT BE INVALID AT THIS POINT! USE anPointer INSTEAD. I love C++

        if (auto anPointer = engine->entities.try_get<Animated>(e))
        {
            for (auto it = anPointer->animationUpdateFunctions.cbegin(); it != anPointer->animationUpdateFunctions.cend(); it++)
                mapCopy[it->first] = it->second;

            anPointer->animationUpdateFunctions = mapCopy;
        }
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
