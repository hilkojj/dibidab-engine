
#include "SaveGame.h"

#include "dibidab.h"

#include "../lua/lua_converters.h"

#include <files/file_utils.h>

#ifndef DIBIDAB_NO_SAVE_GAME

dibidab::SaveGame::SaveGame(const char *path) : loadedFromPath(path ? path : "")
{
    luaTable = sol::table::create(luau::getLuaState().lua_state());

    if (!loadedFromPath.empty() && fu::exists(path))
    {
        auto data = fu::readBinary(path);
        json jsonData = json::from_cbor(data.begin(), data.end());
        jsonToLuaTable(luaTable, jsonData.at("luaTable"));
    }
}

const char *SAVE_GAME_ENTITIES_TABLE_NAME = "saveGameEntities";

void dibidab::SaveGame::save(const char *path)
{
    if (path == nullptr && loadedFromPath.empty())
    {
        return;
    }

    json j = json::object();
    json &jsonLuaTable = j["luaTable"] = json::object();
    jsonFromLuaTable(luaTable, jsonLuaTable);

    std::vector<std::string> idsToRemove;

    for (auto &[id, saveData] : jsonLuaTable[SAVE_GAME_ENTITIES_TABLE_NAME].items())
        if (saveData.empty())
            idsToRemove.push_back(id);

    for (auto &id : idsToRemove)
        jsonLuaTable[SAVE_GAME_ENTITIES_TABLE_NAME].erase(id);

    std::vector<uint8> data;
    json::to_cbor(j, data);
    fu::writeBinary(path == nullptr ? loadedFromPath.c_str() : path, (char *) data.data(), data.size());
}

sol::table dibidab::SaveGame::getSaveDataForEntity(const std::string &entitySaveGameID, bool temporary)
{
    if (temporary)
        return luau::getLuaState()["tempSaveGameEntities"].get_or_create<sol::table>()[entitySaveGameID].get_or_create<sol::table>();

    auto &saveGameLuaTable = dibidab::getCurrentSession().saveGame.luaTable;

    return saveGameLuaTable[SAVE_GAME_ENTITIES_TABLE_NAME].get_or_create<sol::table>()[entitySaveGameID].get_or_create<sol::table>();
}

#endif
