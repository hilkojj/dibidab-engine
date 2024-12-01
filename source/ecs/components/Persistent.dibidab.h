#pragma once
#include "dibidab_header.h"

#include <json.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>

#include <string>

namespace dibidab::ecs
{
    struct Persistent
    {
        dibidab_component;
        dibidab_expose(lua, json);
        entt::entity entityHint = entt::null;
        std::string applyTemplateOnLoad;
        json data = json::object();

        std::vector<std::string> saveComponents;

        bool bSaveName = true;
    };
}
