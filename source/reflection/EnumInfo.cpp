#include "EnumInfo.h"

namespace
{
    std::map<std::string, dibidab::EnumInfo> &getAllEnumInfos()
    {
        static std::map<std::string, dibidab::EnumInfo> infos;
        return infos;
    }
}

const std::map<std::string, dibidab::EnumInfo> &dibidab::getAllEnumInfos()
{
    return ::getAllEnumInfos();
}

const dibidab::EnumInfo *dibidab::findEnumInfo(const char *enumId)
{
    const auto &infos = ::getAllEnumInfos();
    auto it = infos.find(enumId);
    if (it != infos.end())
    {
        return &it->second;
    }
    return nullptr;
}

void dibidab::registerEnumInfo(const dibidab::EnumInfo &info)
{
    ::getAllEnumInfos().insert({ info.id, info });
}
