#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include <cmath>

class Renderer
{
public:

    // Spinner Animation
    static void Spinner(const char* label, float radius, float thickness, ImU32 color)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return;

        ImGuiContext& g = *GImGui;
        const ImGuiID id = window->GetID(label);

        ImVec2 pos = window->DC.CursorPos;
        ImVec2 size((radius) * 2, (radius + g.Style.FramePadding.y) * 2);

        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb, g.Style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, id))
            return;

        window->DrawList->PathClear();

        int num_segments = 30;
        int start = fabs(ImSin(g.Time * 1.8f) * (num_segments - 5));

        const float a_min = IM_PI * 2.0f * ((float)start) / (float)num_segments;
        const float a_max = IM_PI * 2.0f * ((float)num_segments - 3) / (float)num_segments;

        const ImVec2 centre = ImVec2(pos.x + radius, pos.y + radius + g.Style.FramePadding.y);

        for (int i = 0; i < num_segments; i++) {
            const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
            window->DrawList->PathLineTo(ImVec2(centre.x + ImCos(a) * radius, centre.y + ImSin(a) * radius));
        }

        window->DrawList->PathStroke(color, false, thickness);
    }

    void DrawWelcomeMessage(const char* text)
    {
        float rainbowSpeed = 1.0f;

        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);

        for (int i = 0; text[i] != '\0'; ++i) {
            float hue = std::fmod((ImGui::GetTime() * rainbowSpeed + i * 0.1f), 1.0f);
            ImVec4 color = ImColor::HSV(hue, 0.8f, 0.8f);
            ImGui::TextColored(color, "%c", text[i]);
            ImGui::SameLine(0, 0);
        }
        ImGui::NewLine();

        ImGui::PopFont();
    }
};
