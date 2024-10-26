#pragma once
#include "dibidab_header.h"

#include <math/math_utils.h>
#include <input/key_input.h>

struct ImFontConfig;

namespace dibidab
{
    struct GraphicsSettings
    {
      dibidab_json_method(object);
      dibidab_expose(json);

        // Window:
        bool bVSync = true;
        bool bFullscreen = false;
        ivec2 windowSize = ivec2(1600, 900);

        // OpenGL:
        bool bPrintOpenGLMessages = false;
        bool bPrintOpenGLErrors = false;
        int openGLMajorVersion = 4;
        int openGLMinorVersion = 2;

        // ImGui:
        std::string fontPath;
        float fontSize = 16.0f;

      dibidab_expose();
        ImFontConfig *imGuiFontConfig = nullptr;
    };

    struct KeyInputSettings
    {
      dibidab_json_method(object);
      dibidab_expose(json);
        KeyInput::Key toggleDeveloperOptions = GLFW_KEY_F3;
        KeyInput::Key reloadAssets = GLFW_KEY_F4;
        KeyInput::Key toggleFullscreen = GLFW_KEY_F11;
        KeyInput::Key createEntity = GLFW_KEY_INSERT;
        KeyInput::Key inspectEntity = GLFW_KEY_I;
        KeyInput::Key cancel = GLFW_KEY_ESCAPE;
    };

    struct EngineSettings
    {
      dibidab_json_method(object);
      dibidab_expose(json);
        GraphicsSettings graphics;
        KeyInputSettings developerKeyInput;

        bool bShowDeveloperOptions = true;
    };
}
