#pragma once
#include "../PersistentEntityRef.h"

#include "dibidab_header.h"

namespace dibidab::ecs
{
    struct Persistent
    {
        dibidab_component;
        dibidab_expose(lua, json);
        PersistentEntityID persistentId;
        std::string applyTemplateOnLoad;
        json data = json::object();

        // TODO: replace with ComponentInfo ptrs?
        std::vector<std::string> saveComponents;

        bool bSaveName = true;
    };
}
