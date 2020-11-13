
#include <ecs/EntityEngine.h>
#include "AnimationSystem.h"

void AnimationSystem::update(double deltaTime, EntityEngine *engine)
{
    engine->entities.view<Animated>().each([&](Animated &an) {

        for (auto it = an.animationUpdateFunctions.cbegin(); it != an.animationUpdateFunctions.cend();)
        {
            if (it->second(deltaTime))
                an.animationUpdateFunctions.erase(it++);
            else ++it;
        }
    });
}
