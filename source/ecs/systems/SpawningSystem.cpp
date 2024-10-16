
#include "SpawningSystem.h"
#include "../components/Spawning.dibidab.h"

void SpawningSystem::update(double deltaTime, EntityEngine *room)
{
    this->room = room;
    room->entities.view<DespawnAfter>().each([&](auto e, DespawnAfter &despawnAfter) {
        despawnAfter.timer += deltaTime;
        if (despawnAfter.timer >= despawnAfter.time)
            room->entities.destroy(e);
    });
}
