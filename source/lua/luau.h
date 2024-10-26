#pragma once
extern "C" {
    #include "lua.h"
    #include "lauxlib.h"
    #include "lualib.h"
}

#include <utils/gu_error.h>

#include <sol/sol.hpp>

namespace luau
{
    struct Script
    {
        Script(const std::string &path);

        const sol::bytecode &getByteCode();

      private:
        std::string path;
        sol::bytecode bytecode;
    };

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

    sol::environment environmentFromScript(Script &script, sol::environment *parent = nullptr);
}
