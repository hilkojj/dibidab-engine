#pragma once
#include <dibidab_header.h>

#include <string>
#include <memory>

namespace dibidab
{
    struct Player
    {
        dibidab_expose(lua, json);
        int id;
        std::string name;
    };

    typedef std::shared_ptr<Player> Player_ptr;
}
