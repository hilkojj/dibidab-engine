#pragma once

#include <json_fwd.hpp>

#include <map>
#include <string>
#include <vector>

namespace sol
{
    class state;
}

namespace dibidab
{
    struct ComponentInfo;

    struct VariableInfo
    {
        const char *name;
        const char *typeName;
        const bool bLuaExposed;
        const bool bJsonExposed;
    };

    struct StructInfo
    {
        const char *id;

        const ComponentInfo *componentInfo;

        const std::vector<VariableInfo> variables;

        /**
         * Constructs the struct and converts it to json.
         * NOTE: function is nullptr if struct is not exposed to json!
         */
        json (*getDefaultJsonObject)();

        /**
         * Allows the struct to be constructed and accessed in Lua.
         * https://sol2.readthedocs.io/en/latest/usertypes.html
         * NOTE: function is nullptr if struct is not exposed to lua!
         */
        void (*registerLuaUserType)(sol::state &);

        /**
         * Prefer this function over dibidab::findStructInfo when dealing with typenames coming from VariableInfo::typeName.
         * Because those can omit the namespaces of the type, if this struct is in the same namespace.
         */
        const StructInfo *findStructInfoInNamespace(const char *structId) const;
    };

    const std::map<std::string, StructInfo> &getAllStructInfos();

    const StructInfo *findStructInfo(const char *structId);

    void registerStructInfo(const StructInfo &);
}
