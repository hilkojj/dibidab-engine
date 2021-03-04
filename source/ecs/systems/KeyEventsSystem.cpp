
#include <ecs/EntityEngine.h>
#include "KeyEventsSystem.h"
#include "../../generated/Input.hpp"

void KeyEventsSystem::update(double deltaTime, EntityEngine *engine)
{

    engine->entities.view<KeyListener>().each([&] (auto e, const KeyListener &listener) {

        KeyListener cpy = listener;

        for (auto &[name, keyPtr] : cpy.keys)
        {
            if (KeyInput::justPressed(keyPtr->glfwValue) && engine->entities.valid(e))
                engine->emitEntityEvent(e, keyPtr, (name + "_pressed").c_str());  // todo: queue events
            else if (KeyInput::justReleased(keyPtr->glfwValue) && engine->entities.valid(e))
                engine->emitEntityEvent(e, keyPtr, (name + "_released").c_str());
        }
    });

    // todo: dequeue
}

void KeyEventsSystem::init(EntityEngine *engine)
{
    engine->luaEnvironment["listenToKey"] = [engine] (entt::entity e, KeyInput::Key *keyPtr, const std::string &name) {
        engine->entities.get_or_assign<KeyListener>(e).keys[name] = keyPtr;
    };
}
