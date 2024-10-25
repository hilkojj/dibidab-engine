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

        const std::vector<VariableInfo> variables;

        json (*getDefaultJsonObject)();

        void (*registerLuaUserType)(sol::state &);
    };

    const std::map<std::string, StructInfo> &getAllStructInfos();

    const StructInfo *findStructInfo(const char *structNameOrId);

    void registerStructInfo(const StructInfo &);
}
