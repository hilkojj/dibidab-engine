#pragma once
#include "dibidab_settings.dibidab.h"

#include <gu/game_config.h>
#include <utils/delegate.h>

namespace dibidab
{
    namespace level
    {
        class Level;
    }

    struct Config
    {
        gu::Config guConfig;

        struct
        {
            bool bLua = true;
            bool bJson = false;
            bool bShaders = false;
            bool bTextures = false;
        }
        addAssetLoaders;
    };

    gu::Config guConfigFromSettings();

    void init(int argc, char *argv[], Config &config);

    void run();

    void setLevel(level::Level *level);

    level::Level *getLevel();

    extern delegate<void(level::Level *)> onLevelChange;

    extern EngineSettings settings;

    extern std::map<std::string, std::string> startupArgs;
};
