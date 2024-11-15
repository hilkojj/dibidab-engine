#pragma once

#include <utility>
#include <vector>
#include <map>
#include <string>

namespace sol
{
    class state;
}

namespace dibidab
{
    struct EnumInfo
    {
        const char *id;
        std::vector<std::pair<std::string, int>> values;

        void (*registerLuaEnum)(sol::state &);
    };

    const std::map<std::string, EnumInfo> &getAllEnumInfos();

    const EnumInfo *findEnumInfo(const char *enumId);

    void registerEnumInfo(const EnumInfo &);
}
