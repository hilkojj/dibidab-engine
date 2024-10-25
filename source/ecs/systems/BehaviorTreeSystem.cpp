#include "BehaviorTreeSystem.h"

#include "../Engine.h"
#include "../components/Brain.dibidab.h"

struct BrainPendingActivation
{};

void dibidab::ecs::BehaviorTreeSystem::init(Engine *engine)
{
    System::init(engine);

    engine->luaEnvironment["component"]["Brain"]["setBehaviorTreeFor"] = [engine] (entt::entity e, behavior::Tree::Node *node)
    {
        if (!engine->entities.valid(e))
        {
            throw gu_err("Entity " + std::to_string(int(e)) + " is not valid!");
        }
        Brain &brain = engine->entities.get_or_assign<Brain>(e);
        brain.behaviorTree.setRootNode(node);
        engine->entities.assign<BrainPendingActivation>(e);
    };
}

void dibidab::ecs::BehaviorTreeSystem::update(double deltaTime, Engine *engine)
{
    engine->entities.view<BrainPendingActivation>().each([&] (entt::entity e, auto)
    {
        if (Brain *brain = engine->entities.try_get<Brain>(e))
        {
            if (behavior::Tree::Node *rootNode = brain->behaviorTree.getRootNode())
            {
                rootNode->enter();
            }
        }
        engine->entities.remove<BrainPendingActivation>(e);
    });

}
