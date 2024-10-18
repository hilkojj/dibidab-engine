#pragma once

#include <utils/type_name.h>

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

        EntityObserver *(*createObserver)(entt::registry &);

        json (*getDefaultJsonObject)();
        void (*getJsonObject)(entt::entity, const entt::registry &, json &outObject);
        void (*getJsonArray)(entt::entity, const entt::registry &, json &outArray);
        void (*setFromJson)(const json &objectOrArray, entt::entity, entt::registry &);

        void (*setFromLua)(const sol::table &, entt::entity, entt::registry &);
        void (*fillLuaUtilsTable)(sol::table &, entt::registry &, const component_info *);
    };

    const std::map<std::string, component_info> &getAllComponentInfos();

    const component_info *findComponentInfo(const char *name);

    template <typename component>
    const component_info *findComponentInfo()
    {
        return findComponentInfo(typename_utils::getTypeName<component>());
    }

    const component_info *getInfoFromUtilsTable(const sol::table &);

    void registerComponentInfo(const component_info &);
}
