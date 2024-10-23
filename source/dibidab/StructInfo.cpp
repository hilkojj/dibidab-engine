#include "StructInfo.h"

#include <map>

namespace
{
    std::map<std::string, dibidab::StructInfo> &getAllStructInfos()
    {
        static std::map<std::string, dibidab::StructInfo> infos;
        return infos;
    }
}

const std::map<std::string, dibidab::StructInfo> &dibidab::getAllStructInfos()
{
    return ::getAllStructInfos();
}

const dibidab::StructInfo *dibidab::findStructInfo(const char *structId)
{
    const auto &infos = ::getAllStructInfos();
    auto it = infos.find(structId);
    if (it != infos.end())
    {
        return &it->second;
    }
    return nullptr;
}

void dibidab::registerStructInfo(const dibidab::StructInfo &info)
{
    ::getAllStructInfos().insert({ info.id, info });
}
