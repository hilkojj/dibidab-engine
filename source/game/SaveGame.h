#pragma once
#include "lua/luau.h"

#include <json.hpp>

#ifndef DIBIDAB_NO_SAVE_GAME

namespace dibidab
{
    struct SaveGame
    {
        SaveGame(const char *path);

        sol::table luaTable;

        void save(const char *path = nullptr); // if path == nullptr then same path from constructor is used.

        static sol::table getSaveDataForEntity(const std::string &entitySaveGameID, bool bTemporary);

      private:
        std::string loadedFromPath;
    };
}
#endif
