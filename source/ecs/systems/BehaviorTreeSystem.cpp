
#include "BehaviorTreeSystem.h"

#include "../EntityEngine.h"
#include "../../ai/behavior_trees/Tree.h"

#include "../components/Brain.dibidab.h"

struct BrainPendingActivation
{};

void BehaviorTreeSystem::init(EntityEngine *engine)
{
    EntitySystem::init(engine);

    engine->luaEnvironment["component"]["Brain"]["setBehaviorTreeFor"] = [engine] (entt::entity e, BehaviorTree::Node *node)
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

void BehaviorTreeSystem::update(double deltaTime, EntityEngine *engine)
{
    engine->entities.view<BrainPendingActivation>().each([&] (entt::entity e, auto)
    {
        if (Brain *brain = engine->entities.try_get<Brain>(e))
        {
            if (BehaviorTree::Node *rootNode = brain->behaviorTree.getRootNode())
            {
                rootNode->enter();
            }
        }
        engine->entities.remove<BrainPendingActivation>(e);
    });

}
