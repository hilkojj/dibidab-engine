#pragma once
#include <dibidab_header.h>

struct PlayerControlled
{
  dibidab_component;
  dibidab_expose(lua, json);
    int playerId = -1;
};


struct LocalPlayer
{
  dibidab_component;
};