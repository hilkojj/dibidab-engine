#pragma once
#include "Session.h"

namespace dibidab
{
    class SingleplayerSession : public Session
    {
        bool firstUpdate = true;

      public:

        SingleplayerSession(const char *saveGamePath);;

        void join(std::string username) override;

        void update(double deltaTime) override;

        void setLevel(level::Level *);

    };
}
