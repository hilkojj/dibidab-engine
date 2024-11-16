#pragma once

#include <utils/type_name.h>

#include <entt/entity/fwd.hpp>
#include <json_fwd.hpp>
#include <sol/forward.hpp>

#include <map>

namespace dibidab
{
    namespace ecs
    {
        class Observer;
    }

    struct ComponentInfo
    {
        const char *name;
        const char *structId;

        const std::vector<const char *> categoryPath;

        bool (*hasComponent)(entt::entity, const entt::registry &);
        void (*addComponent)(entt::entity, entt::registry &);
        void (*removeComponent)(entt::entity, entt::registry &);

        ecs::Observer *(*createObserver)(entt::registry &);

        /**
         *  Gets the component on the entity (check presence first with `hasComponent`!)
         *  and returns it as a Json object.
         *  NOTE: function is nullptr if component is not exposed to json!
         */
        void (*getJsonObject)(entt::entity, const entt::registry &, json &outObject);

        /**
         *  Gets the component on the entity (check presence first with `hasComponent`!)
         *  and returns it as a Json array (similar as object, but omitting keys).
         *  NOTE: function is nullptr if component is not exposed to json!
         */
        void (*getJsonArray)(entt::entity, const entt::registry &, json &outArray);

        /**
         *  Sets/replaces the component on the entity.
         *  NOTE: function is nullptr if component is not exposed to json!
         */
        void (*setFromJson)(const json &objectOrArray, entt::entity, entt::registry &);

        /**
         *  Sets/replaces the component on the entity.
         *  If the component was already present, it will be used as the base on which the Json is applied as a patch.
         *  Which means that all non-json-exposed values are left intact.
         *  NOTE: function is nullptr if component is not exposed to json!
         */
        void (*patchFromJson)(const json &objectOrArray, entt::entity, entt::registry &);

        void (*setFromLua)(const sol::table &, entt::entity, entt::registry &);
        void (*fillLuaUtilsTable)(sol::table &, entt::registry &, const ComponentInfo *);
    };

    const std::map<std::string, ComponentInfo> &getAllComponentInfos();

    const ComponentInfo *findComponentInfo(const char *name);

    template <typename Component>
    const ComponentInfo *findComponentInfo()
    {
        return findComponentInfo(typename_utils::getTypeName<Component>().c_str());
    }

    const ComponentInfo *getInfoFromUtilsTable(const sol::table &);

    void registerComponentInfo(const ComponentInfo &);
}
