#pragma once
#include "luau.h"

#include <entt/entity/registry.hpp>
#include <assets/asset.h>
#include <json.hpp>

/////////// asset<>

template <typename Handler, typename type>
bool sol_lua_check(sol::types<asset<type>>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking) {

    bool success = sol::stack::check<const char *>(L, index, handler);
    tracking.use(1);
    return success;
}

template<typename type>
asset<type> sol_lua_get(sol::types<asset<type>>, lua_State* L, int index, sol::stack::record& tracking)
{
    sol::optional<const char *> path = sol::stack::get<sol::optional<const char *>>(L, index);

    tracking.use(1);

    asset<type> a;

    if (path.has_value())
        a.set(path.value());

    return a;
}

template<typename type>
int sol_lua_push(sol::types<asset<type>>, lua_State* L, const asset<type>& asset) {

    if (asset.isSet())
        sol::stack::push(L, asset.getLoadedAsset()->shortPath);
    else
        sol::stack::push(L, sol::nil);

    return 1;
}
///////////

/////////// entt::entity

template <typename Handler>
bool sol_lua_check(sol::types<entt::entity>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking) {

    return sol::stack::check<sol::optional<int>>(L, index, handler, tracking);
}

entt::entity sol_lua_get(sol::types<entt::entity>, lua_State* L, int index, sol::stack::record& tracking);

int sol_lua_push(sol::types<entt::entity>, lua_State* L, const entt::entity& e);

///////////////

void jsonFromLuaTable(const sol::table &table, json &jsonOut);

void jsonToLuaTable(sol::table &table, const json &json);

template <>
struct sol::usertype_container<json> : public container_detail::usertype_container_default<json>
{
    static int index_get(lua_State *lua)
    {
        json &j = *sol::stack::unqualified_check_get<json *>(lua, 1).value();

        json *jOut = nullptr;

        const char *keyStr = sol::stack::unqualified_check_get<const char *>(lua, 2).value_or((const char *) nullptr);
        if (keyStr)
            jOut = &j[keyStr];

        else
        {
            int keyInt = sol::stack::unqualified_check_get<int>(lua, 2).value();
            jOut = &j[keyInt];
        }
        if (jOut->is_structured())
            sol::stack::push(lua, jOut);
        else if (jOut->is_boolean())
            sol::stack::push(lua, bool(*jOut));
        else if (jOut->is_null())
            return 0;
        else if (jOut->is_number_float())
            sol::stack::push(lua, float(*jOut));
        else if (jOut->is_number())
            sol::stack::push(lua, int(*jOut));
        else if (jOut->is_string())
            sol::stack::push(lua, std::string(*jOut));
        return 1;
    }


    static int index_set(lua_State *lua)
    {
        json &j = *sol::stack::unqualified_check_get<json *>(lua, 1).value();

        sol::object luaVal = sol::stack::unqualified_check_get<sol::object>(lua, 3).value();

        json jsonVal;

        switch (luaVal.get_type())  // copied from jsonFromLuaTable(). todo: move this switch to separate function
        {
            case sol::type::number:
                jsonVal = luaVal.as<double>();
                break;
            case sol::type::boolean:
                jsonVal = luaVal.as<bool>();
                break;
            case sol::type::string:
                jsonVal = luaVal.as<std::string>();
                break;
            case sol::type::table:
                jsonFromLuaTable(luaVal.as<sol::table>(), jsonVal);
                break;
            default:
                break;
        }

        const char *keyStr = sol::stack::unqualified_check_get<const char *>(lua, 2).value_or((const char *) nullptr);
        if (keyStr)
            j[keyStr] = jsonVal;

        else
        {
            int keyInt = sol::stack::unqualified_check_get<int>(lua, 2).value();
            j[keyInt] = jsonVal;
        }
        return 1;
    }
};
