#pragma once
#include "Session.h"

class SingleplayerSession : public Session
{

    bool firstUpdate = true;

  public:

    SingleplayerSession(const char *saveGamePath) : Session(saveGamePath) {};

    void join(std::string username) override;

    void update(double deltaTime) override;

    void setLevel(Level *);

};
