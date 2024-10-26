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

        json (*getDefaultJsonObject)();

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
