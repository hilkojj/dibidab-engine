
#include "dibidab/struct.h"
#include "game/dibidab.h"
#include "game/session/SingleplayerSession.h"
#include "ecs/EntityInspector.h"

#include <gu/game_utils.h>
#include <gu/game_config.h>

int main(int argc, char *argv[])
{
    gu::Config configgy;
    dibidab::addDefaultAssetLoaders();
    dibidab::init(argc, argv, configgy);

    std::cout << dibidab::findStructInfo("dibidab::EngineSettings")->id << std::endl;

    SingleplayerSession session(nullptr);
    dibidab::setCurrentSession(&session);
    session.join("yoooooooooooooo");

    Level level;
    level.bSaveOnDestruct = false;
    Room *room = new Room;
    level.addRoom(room);

    session.setLevel(&level);

    EntityInspector inspector(*room);

    auto renderInspector = gu::beforeRender += [&] (double deltaTime)
    {
        inspector.draw();
    };

    dibidab::run();
    return 0;
}
