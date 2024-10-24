#pragma once

#include <utils/type_name.h>

#include <entt/entity/fwd.hpp>
#include <json_fwd.hpp>
#include <sol/forward.hpp>

#include <map>

class EntityObserver;


namespace dibidab
{
    struct ComponentInfo
    {
        const char *name;
        const char *structId;

        const std::vector<const char *> categoryPath;

        bool (*hasComponent)(entt::entity, const entt::registry &);
        void (*addComponent)(entt::entity, entt::registry &);
        void (*removeComponent)(entt::entity, entt::registry &);

        EntityObserver *(*createObserver)(entt::registry &);

        void (*getJsonObject)(entt::entity, const entt::registry &, json &outObject);
        void (*getJsonArray)(entt::entity, const entt::registry &, json &outArray);
        void (*setFromJson)(const json &objectOrArray, entt::entity, entt::registry &);

        void (*setFromLua)(const sol::table &, entt::entity, entt::registry &);
        void (*fillLuaUtilsTable)(sol::table &, entt::registry &, const ComponentInfo *);
    };

    const std::map<std::string, ComponentInfo> &getAllComponentInfos();

    const ComponentInfo *findComponentInfo(const char *name);

    template <typename component>
    const ComponentInfo *findComponentInfo()
    {
        return findComponentInfo(typename_utils::getTypeName<component>());
    }

    const ComponentInfo *getInfoFromUtilsTable(const sol::table &);

    void registerComponentInfo(const ComponentInfo &);
}
