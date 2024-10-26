#include "game/dibidab.h"
#include "ecs/Inspector.h"
#include "level/Level.h"

#include <gu/game_utils.h>

int main(int argc, char *argv[])
{
    dibidab::Config config;
    config.guConfig = dibidab::guConfigFromSettings();

    dibidab::init(argc, argv, config);

    dibidab::level::Level level;
    level.bSaveOnDestruct = false;
    dibidab::level::Room *room = new dibidab::level::Room;
    level.addRoom(room);

    dibidab::setLevel(&level);

    dibidab::ecs::Inspector inspector(*room);

    auto renderInspector = gu::beforeRender += [&] (double deltaTime)
    {
        inspector.draw();
    };

    dibidab::run();
    return 0;
}
