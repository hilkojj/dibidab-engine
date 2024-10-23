#include "struct.h"

#include <map>

namespace
{
    std::map<std::string, dibidab::struct_info> &getAllStructInfos()
    {
        static std::map<std::string, dibidab::struct_info> infos;
        return infos;
    }
}

const std::map<std::string, dibidab::struct_info> &dibidab::getAllStructInfos()
{
    return ::getAllStructInfos();
}

const dibidab::struct_info *dibidab::findStructInfo(const char *structId)
{
    const auto &infos = ::getAllStructInfos();
    auto it = infos.find(structId);
    if (it != infos.end())
    {
        return &it->second;
    }
    return nullptr;
}

void dibidab::registerStructInfo(const dibidab::struct_info &info)
{
    ::getAllStructInfos().insert({ info.id, info });
}
