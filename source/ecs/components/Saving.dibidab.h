#pragma once
#include "../PersistentEntityRef.h"

#include <dibidab_header.h>

struct Persistent
{
  dibidab_component;
  dibidab_expose(lua, json);
    PersistentEntityID persistentId;
    std::string applyTemplateOnLoad;
    json data = json::object();

    bool saveFinalPosition = false;
    bool saveSpawnPosition = false;
    vec3 spawnPosition = vec3(0.0f);

    bool revive = false;
    bool saveAllComponents = false;
    std::vector<std::string> saveComponents;

    bool saveName = true;
};
