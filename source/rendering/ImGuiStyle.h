#pragma once
#include "../dibidab/dibidab.h"

#include <imgui.h>

inline void setImGuiStyleAndConfig()
{
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavNoCaptureKeyboard;

    if (!dibidab::settings.graphics.fontPath.empty())
    {
        ImGui::GetIO().Fonts->AddFontFromFileTTF(dibidab::settings.graphics.fontPath.c_str(),
            dibidab::settings.graphics.fontSize, dibidab::settings.graphics.imGuiFontConfig);
    }
    ImGui::GetIO().Fonts->AddFontDefault();
}
