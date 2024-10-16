#pragma once
#include <dibidab_header.h>

#include <entt/entity/fwd.hpp>

#include <string>
#include <map>
#include <vector>

struct Parent
{
  dibidab_component;
  dibidab_expose(lua, json);
    bool deleteChildrenOnDeletion = true;
    std::vector<entt::entity> children;
    std::map<std::string, entt::entity> childrenByName;
};

struct Child
{
  dibidab_component;
  dibidab_expose(lua, json);
    entt::entity parent;
    std::string name;
};
