#pragma once

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

    const struct_info *findStructInfo(const char *structId);

    void registerStructInfo(const struct_info &);
}
