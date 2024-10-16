#pragma once
#include <unordered_map>
#include <functional>
#include <math/interpolation.h>

struct Animated
{
    /*
     * {
     *      "nameOfAnimatedField": animationUpdateFunction(float deltaTime) -> bool (animation finished)
     * }
     */
    std::unordered_map<std::string, std::function<bool(float deltaTime)>> animationUpdateFunctions;
};
