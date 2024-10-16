#pragma once
#include <input/key_input.h>
#include <input/gamepad_input.h>

#include <dibidab_header.h>

struct KeyListener
{
  dibidab_component;
    std::map<std::string, KeyInput::Key *> keys;
};


struct GamepadListener
{
  dibidab_component;
  dibidab_expose(lua, json);
    uint gamepad;

  dibidab_expose();
    std::map<std::string, GamepadInput::Button *> buttons;
};
