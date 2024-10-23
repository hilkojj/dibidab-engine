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
    bool bDestroyChildrenOnDestroy = true;
    std::vector<entt::entity> children;
    std::map<std::string, entt::entity> childrenByName;
};

struct Child
{
  dibidab_component;
  dibidab_expose(lua, json);
    entt::entity parent;
    std::string name;

    Parent testSub;
    std::vector<Parent> testSubVector { Parent { true }, Parent { false } };
    std::vector<entt::entity> entityVector { entt::null };
    std::vector<std::string> stringVector { "test string" };
    std::vector<vec3> positionVector { vec3(1, 2, 3) };
    std::map<int, Parent> testSubMap {
        { 2, Parent() },
        { 5, Parent() },
    };
    std::map<std::string, Parent> testSubMapStr {
        { "test0", Parent() },
        { "test1", Parent() },
    };
};
