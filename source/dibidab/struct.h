#pragma once

#include <map>
#include <string>

namespace sol
{
    class state;
}

namespace dibidab
{
    struct struct_info
    {
        const char *id;

        void (*registerLuaUserType)(sol::state &);
    };

    const std::map<std::string, struct_info> &getAllStructInfos();

    const struct_info *findStructInfo(const char *structId);

    void registerStructInfo(const struct_info &);
}
