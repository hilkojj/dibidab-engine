
#include "Session.h"
#include "../../ecs/templates/Template.h"
#include "../../ecs/components/PlayerControlled.dibidab.h"

const dibidab::Player_ptr &dibidab::Session::getPlayerById(int id) const
{
    for (auto &p : players) if (p->id == id) return p;
    throw gu_err("Player " + std::to_string(id) + " does not exist!");
}

void dibidab::Session::validateUsername(const std::string &name, std::string &declineReason) const
{
    for (const char &c : name)
        if (c < ' ' || c > '~') // http://www.asciitable.com/
            declineReason = "Please use normal characters in your name.\n";

    if (name.empty() || name.size() > 32)
        declineReason = "Username length must be 1-32.";

    for (auto &p : players)
        if (p->name == name)
            declineReason = "Username already taken. :(";
}

dibidab::Player_ptr dibidab::Session::deletePlayer(int pId, std::list<Player_ptr> &players)
{
    Player_ptr deletedPlayer;
    players.remove_if([&](auto &p) {
        if (pId == p->id)
        {
            deletedPlayer = p;
            return true;
        }
        return false;
    });
    if (!deletedPlayer) throw gu_err("Player not found");
    return deletedPlayer;
}

dibidab::Player_ptr dibidab::Session::getPlayer(int id) const
{
    for (auto &p : players) if (p->id == id) return p;
    return nullptr;
}

dibidab::Session::Session(const char *saveGamePath)
#ifndef DIBIDAB_NO_SAVE_GAME
    : saveGame(saveGamePath)
#endif
{

}

void dibidab::Session::spawnPlayerEntities()
{
    assert(level != nullptr);
    for (auto &player : players)
        spawnPlayerEntity(player);
}

void dibidab::Session::spawnPlayerEntity(Player_ptr &p)
{
    if (level->getNrOfRooms() >= 1)
    {
        dibidab::level::Room *spawnRoom = level->getRoomByName(level->spawnRoom.c_str());
        if (!spawnRoom)
            spawnRoom = &level->getRoom(0);

        auto &templ = spawnRoom->getTemplate("Player");
        auto e = templ.create();
        dibidab::ecs::PlayerControlled pc;
        pc.playerId = p->id;
        spawnRoom->entities.assign<ecs::PlayerControlled>(e, pc);
        if (p == localPlayer)
            spawnRoom->entities.assign<ecs::LocalPlayer>(e);
    }
    else throw gu_err("Cant spawn Player entity in an empty Level!");
}

void dibidab::Session::removePlayerEntities(int playerId)
{
    for (int i = 0; i < level->getNrOfRooms(); i++)
    {
        level::Room &r = level->getRoom(i);
        r.entities.view<ecs::PlayerControlled>().each([&] (entt::entity e, ecs::PlayerControlled &pC) {
            if (pC.playerId == playerId)
                r.entities.destroy(e);
        });
    }
}

dibidab::Session::~Session()
{
    delete level;
#ifndef DIBIDAB_NO_SAVE_GAME
    saveGame.save();
#endif
}
