#pragma once
#include <dibidab_header.h>

#include <math/math_utils.h>

struct Position3d
{
  dibidab_component;
  dibidab_expose(lua, json);
    vec3 vec = vec3();
};
