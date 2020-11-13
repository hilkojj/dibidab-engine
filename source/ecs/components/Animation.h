
#ifndef GAME_ANIMATION_H
#define GAME_ANIMATION_H

#include <unordered_map>
#include <functional>
#include <utils/math/interpolation.h>

struct Animated
{
    /*
     * {
     *      "nameOfAnimatedField": animationUpdateFunction(float deltaTime) -> bool (animation finished)
     * }
     */
    std::unordered_map<std::string, std::function<bool(float deltaTime)>> animationUpdateFunctions;
};

#endif //GAME_ANIMATION_H
