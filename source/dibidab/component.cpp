#include "component.h"

#include <utils/gu_error.h>

namespace
{
    std::map<const char *, dibidab::component_info> &getAllComponentInfos()
    {
        static std::map<const char *, dibidab::component_info> infos;
        return infos;
    }
}

const std::map<const char *, dibidab::component_info> &dibidab::getAllComponentInfos()
{
    return ::getAllComponentInfos();
}

const dibidab::component_info *dibidab::findComponentInfo(const char *name)
{
    const auto &infos = getAllComponentInfos();
    auto it = infos.find(name);
    return it == infos.end() ? nullptr : &it->second;
}

void dibidab::registerComponentInfo(const dibidab::component_info &info)
{
    if (findComponentInfo(info.name) != nullptr)
    {
        throw gu_err(std::string("Already registered ") + info.name);
    }
    ::getAllComponentInfos()[info.name] = info;
}