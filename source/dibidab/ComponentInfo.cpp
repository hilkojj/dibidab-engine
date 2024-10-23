#include "ComponentInfo.h"

#include <utils/gu_error.h>

#include <sol/sol.hpp>

namespace
{
    std::map<std::string, dibidab::ComponentInfo> &getAllComponentInfos()
    {
        static std::map<std::string, dibidab::ComponentInfo> infos;
        return infos;
    }
}

const std::map<std::string, dibidab::ComponentInfo> &dibidab::getAllComponentInfos()
{
    return ::getAllComponentInfos();
}

const dibidab::ComponentInfo *dibidab::findComponentInfo(const char *name)
{
    const auto &infos = getAllComponentInfos();
    auto it = infos.find(name);
    return it == infos.end() ? nullptr : &it->second;
}

const dibidab::ComponentInfo *dibidab::getInfoFromUtilsTable(const sol::table &table)
{
    return table["info"];
}

void dibidab::registerComponentInfo(const dibidab::ComponentInfo &info)
{
    if (findComponentInfo(info.name) != nullptr)
    {
        throw gu_err(std::string("Already registered ") + info.name);
    }
    ::getAllComponentInfos().insert({ info.name, info });
}
