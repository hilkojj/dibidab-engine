
#ifndef GAME_LUAU_H
#define GAME_LUAU_H

extern "C" {
    #include "lua.h"
    #include "lauxlib.h"
    #include "lualib.h"
}
#define SOL_ALL_SAFETIES_ON 1

#include "../external/lua/sol2/single/include/sol/sol.hpp"

namespace luau
{
    sol::state &getLuaState();

    struct Script
    {
        Script(const std::string &path);

        const sol::bytecode &getByteCode();

      private:
        std::string path;
        sol::bytecode bytecode;
    };

}

#endif //GAME_LUAU_H
