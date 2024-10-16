#pragma once

#include <entt/entity/fwd.hpp>
#include <json_fwd.hpp>
#include <sol/forward.hpp>

#include <map>

class EntityObserver;


namespace dibidab
{
    struct component_info
    {
        const char *name;
        const char *structId;

        bool (*hasComponent)(entt::entity, const entt::registry &);
        void (*addComponent)(entt::entity, entt::registry &);
        void (*removeComponent)(entt::entity, entt::registry &);

        EntityObserver (*createObserver)(entt::registry &);

        json (*getDefaultJsonObject)();
        void (*getJsonObject)(entt::entity, const entt::registry &, json &outObject);
        void (*getJsonArray)(entt::entity, const entt::registry &, json &outArray);
        void (*setFromJson)(const json &objectOrArray, entt::entity, entt::registry &);

        void (*setFromLua)(const sol::table &, entt::entity, entt::registry &);
        void (*fillLuaUtilsTable)(sol::table &, entt::registry &);
    };

    const std::map<const char *, component_info> &getAllComponentInfos();

    const component_info *findComponentInfo(const char *name);

    void registerComponentInfo(const component_info &);
}
