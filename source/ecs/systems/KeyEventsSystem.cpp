
#include <ecs/EntityEngine.h>
#include "KeyEventsSystem.h"
#include "../components/Input.dibidab.h"

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

    engine->entities.view<GamepadListener>().each([&](auto e, GamepadListener &listener) {

        GamepadListener cpy = listener;

        for (auto &[name, keyPtr] : cpy.buttons)
        {
            if (GamepadInput::justPressed(cpy.gamepad, keyPtr->glfwValue) && engine->entities.valid(e))
                engine->emitEntityEvent(e, keyPtr, (name + "_pressed").c_str());  // todo: queue events
            else if (GamepadInput::justReleased(cpy.gamepad, keyPtr->glfwValue) && engine->entities.valid(e))
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
    engine->luaEnvironment["listenToGamepadButton"] = [engine](entt::entity e, uint gamepad, GamepadInput::Button *buttonPtr, const std::string &name) {
        auto &l = engine->entities.get_or_assign<GamepadListener>(e);
        l.gamepad = gamepad; // todo: prev caller to listenToGamepadButton still expects events from previous gamepad...
        l.buttons[name] = buttonPtr;
    };
    engine->luaEnvironment["getGamepadAxis"] = [](uint gamepad, const GamepadInput::Axis &axis) {
        return GamepadInput::getAxis(gamepad, axis.glfwValue);
    };
}
