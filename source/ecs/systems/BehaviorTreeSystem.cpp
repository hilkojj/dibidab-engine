#include "BehaviorTreeSystem.h"

#include "../Engine.h"
#include "../components/Behavior.dibidab.h"

struct TreePendingActivation
{};

void dibidab::ecs::BehaviorTreeSystem::init(Engine *engine)
{
    System::init(engine);

    engine->luaEnvironment["component"]["Behavior"]["setTreeFor"] = [engine] (entt::entity e, behavior::Tree::Node *node)
    {
        if (!engine->entities.valid(e))
        {
            throw gu_err("Entity " + std::to_string(int(e)) + " is not valid!");
        }
        Behavior &behavior = engine->entities.get_or_assign<Behavior>(e);
        behavior.tree.setRootNode(node);
        engine->entities.assign<TreePendingActivation>(e);
    };
}

void dibidab::ecs::BehaviorTreeSystem::update(double deltaTime, Engine *engine)
{
    engine->entities.view<TreePendingActivation>().each([&] (entt::entity e, auto)
    {
        if (Behavior *behavior = engine->entities.try_get<Behavior>(e))
        {
            if (behavior::Tree::Node *rootNode = behavior->tree.getRootNode())
            {
                rootNode->enter();
            }
        }
        engine->entities.remove<TreePendingActivation>(e);
    });

}
