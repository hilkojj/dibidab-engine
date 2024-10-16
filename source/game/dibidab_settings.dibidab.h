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
        bool vsync = true;
        bool fullscreen = false;
        ivec2 windowSize = ivec2(1600, 900);

        // OpenGL:
        bool printOpenGLMessages = false;
        bool printOpenGLErrors = false;
        int openGLMajorVersion = 4;
        int openGLMinorVersion = 2;

        // ImGui:
        u8vec3 imGuiThemeColor_background = vec3(37, 33, 49);
        u8vec3 imGuiThemeColor_text = vec3(244, 241, 222);
        u8vec3 imGuiThemeColor_main = vec3(218, 17, 94);
        u8vec3 imGuiThemeColor_mainAccent = vec3(121, 35, 89);
        u8vec3 imGuiThemeColor_highLight = vec3(199, 239, 0);
        std::string imGuiFont;
        float imGuiFontSize = 16.0f;

      dibidab_expose();
        ImFontConfig *imGuiFontConfig = nullptr;
    };


    struct KeyInputSettings
    {
      dibidab_json_method(object);
      dibidab_expose(json);
        KeyInput::Key reloadAssets = GLFW_KEY_F4;
        KeyInput::Key toggleFullscreen = GLFW_KEY_F11;
        KeyInput::Key toggleDeveloperOptions = GLFW_KEY_F3;
        KeyInput::Key inspectEntity = GLFW_KEY_I;
        KeyInput::Key moveEntity = GLFW_KEY_M;
        KeyInput::Key moveEntityAndSpawnPoint = GLFW_KEY_LEFT_ALT;
        KeyInput::Key createEntity = GLFW_KEY_INSERT;
    };

    struct EngineSettings
    {
      dibidab_json_method(object);
      dibidab_expose(json);
        GraphicsSettings graphics;
        KeyInputSettings keyInput;

        bool bShowDeveloperOptions = true;
        bool bLimitUpdatesPerSec = false;
    };
}
