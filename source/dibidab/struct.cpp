#include "struct.h"

#include <map>
#include <string>

namespace
{
    std::map<std::string, dibidab::struct_info> structInfoByName;
}

const dibidab::struct_info *dibidab::findStructInfo(const char *structId)
{
    auto it = structInfoByName.find(structId);
    if (it != structInfoByName.end())
    {
        return &it->second;
    }
    return nullptr;
}

void dibidab::registerStructInfo(const dibidab::struct_info &info)
{
    structInfoByName[info.id] = info;
}

