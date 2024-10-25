
#include "reflection/StructInfo.h"
#include "game/dibidab.h"
#include "game/session/SingleplayerSession.h"
#include "ecs/Inspector.h"

#include <gu/game_utils.h>
#include <gu/game_config.h>

int main(int argc, char *argv[])
{
    gu::Config configgy;
    dibidab::addDefaultAssetLoaders();
    dibidab::init(argc, argv, configgy);

    std::cout << dibidab::findStructInfo("dibidab::EngineSettings")->id << std::endl;

    dibidab::SingleplayerSession session(nullptr);
    dibidab::setCurrentSession(&session);
    session.join("yoooooooooooooo");

    dibidab::level::Level level;
    level.bSaveOnDestruct = false;
    dibidab::level::Room *room = new dibidab::level::Room;
    level.addRoom(room);

    session.setLevel(&level);

    dibidab::ecs::Inspector inspector(*room);

    auto renderInspector = gu::beforeRender += [&] (double deltaTime)
    {
        inspector.draw();
    };

    dibidab::run();
    return 0;
}
