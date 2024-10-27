#include "SpawningSystem.h"

#include "../components/DespawnAfter.dibidab.h"
#include "../Engine.h"

void dibidab::ecs::SpawningSystem::update(double deltaTime, Engine *engine)
{
    engine->entities.view<DespawnAfter>().each([&] (auto e, DespawnAfter &despawnAfter)
    {
        despawnAfter.timer += deltaTime;
        if (despawnAfter.timer >= despawnAfter.time)
        {
            engine->entities.destroy(e);
        }
    });
}
