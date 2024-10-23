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
    struct variable_info
    {
        const char *name;
        const char *typeName;
        const bool bLuaExposed;
        const bool bJsonExposed;
    };

    struct struct_info
    {
        const char *id;

        const std::vector<variable_info> variables;

        json (*getDefaultJsonObject)();

        void (*registerLuaUserType)(sol::state &);
    };

    const std::map<std::string, struct_info> &getAllStructInfos();

    const struct_info *findStructInfo(const char *structId);

    void registerStructInfo(const struct_info &);
}
