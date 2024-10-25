#include "StructInfo.h"

#include <utils/string_utils.h>
#include <utils/gu_error.h>

#include <map>

namespace
{
    std::map<std::string, dibidab::StructInfo> &getAllStructInfos()
    {
        static std::map<std::string, dibidab::StructInfo> infos;
        return infos;
    }
}

const dibidab::StructInfo *dibidab::StructInfo::findStructInfoInNamespace(const char *structId) const
{
    std::vector<std::string> namespaces = su::split(id, "::");
    do
    {
        namespaces.pop_back();
        std::string tryStructId = "";
        for (const std::string &space : namespaces)
        {
            tryStructId += space + "::";
        }
        tryStructId += structId;
        if (const dibidab::StructInfo *foundStruct = findStructInfo(tryStructId.c_str()))
        {
            return foundStruct;
        }
    }
    while (!namespaces.empty());
    return nullptr;
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
