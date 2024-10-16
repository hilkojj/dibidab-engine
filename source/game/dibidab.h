#pragma once
#include "session/Session.h"
#include "dibidab_settings.dibidab.h"

namespace gu
{
    struct Config;
}

namespace dibidab
{

    extern EngineSettings settings;

    extern std::map<std::string, std::string> startupArgs;

    extern delegate<void()> onSessionChange;

    Session &getCurrentSession();
    Session *tryGetCurrentSession();
    void setCurrentSession(Session *);

    void addDefaultAssetLoaders();

    void init(int argc, char *argv[], gu::Config &config);

    void run();
};
