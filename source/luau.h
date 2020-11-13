
#ifndef GAME_LUAU_H
#define GAME_LUAU_H

extern "C" {
    #include "lua.h"
    #include "lauxlib.h"
    #include "lualib.h"
}
#define SOL_ALL_SAFETIES_ON 1

#include <sol/sol.hpp>
#include <utils/gu_error.h>

namespace luau
{
    sol::state &getLuaState();

    template <typename ...Args>
    void callFunction(sol::function func, Args&&... args)
    {
        sol::protected_function_result result = func(std::forward<Args>(args)...);
        if (!result.valid())
            throw gu_err(result.get<sol::error>().what());
    }

    template <typename ...Args>
    void tryCallFunction(sol::function func, Args&&... args)
    {
        try
        {
            callFunction(func, std::forward<Args>(args)...);
        }
        catch (std::exception &exc)
        {
            std::cerr << "Error while calling Lua function:\n";
            std::cerr << exc.what() << std::endl;
        }
    }

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
