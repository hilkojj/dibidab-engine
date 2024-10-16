#pragma once
#include <json.hpp>
#include "../luau.h"

#ifndef DIBIDAB_NO_SAVE_GAME

struct SaveGame
{
    SaveGame(const char *path);

    sol::table luaTable;

    void save(const char *path=nullptr); // if path == nullptr then same path from constructor is used.

    static sol::table getSaveDataForEntity(const std::string &entitySaveGameID, bool temporary);

  private:
    std::string loadedFromPath;
};
#endif
