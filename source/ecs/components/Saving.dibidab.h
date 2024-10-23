#pragma once
#include "ecs/PersistentEntityRef.h"

#include "dibidab_header.h"

struct Persistent
{
  dibidab_component;
  dibidab_expose(lua, json);
    PersistentEntityID persistentId;
    std::string applyTemplateOnLoad;
    json data = json::object();

    // TODO: replace with component_info ptrs?
    std::vector<std::string> saveComponents;

    bool bSaveName = true;
};
