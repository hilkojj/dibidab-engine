#pragma once
#include <dibidab_header.h>

#include <json.hpp>
#include <imgui.h>
#include <entt/entity/fwd.hpp>
#include <entt/entity/entity.hpp>

#include <string>

struct InspectPathState
{
    bool multiline = false;
    std::string newKey;
};


struct Inspecting
{
  dibidab_component;
    bool show = true;
    json addingComponentJson;
    std::string addingComponentTypeName;

    std::vector<std::string> currentPath;
    std::map<std::string, InspectPathState> state;

    ImVec2 windowPos = ImVec2(-1, -1);

    entt::entity addInspectingTo = entt::null;

    InspectPathState &getState()
    {
        std::string pathKey;
        for (const std::string &s : currentPath)
        {
            pathKey += "--->" + s;
        }
        return state[pathKey];
    }
};


struct InspectingBrain
{
  dibidab_component;
};
