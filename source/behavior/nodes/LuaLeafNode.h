#pragma once

#include "../Tree.h"

#include <sol/sol.hpp>

namespace dibidab::behavior
{
    struct LuaLeafNode : public Tree::LeafNode
    {
        LuaLeafNode();

        void enter() override;

        void abort() override;

        void finish(Result result) override;

        const char *getName() const override;

        sol::function luaEnterFunction;
        sol::function luaAbortFunction;

      private:
        void finishAborted();

        bool bInEnterFunction;
    };
}
