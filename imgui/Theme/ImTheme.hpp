// ImTheme.hpp

#pragma once

#ifndef IM_THEME_HPP
#define IM_THEME_HPP

#include "imgui.h"

class ImTheme {
public:
    static const void SetStyleSimple()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        // Set global style settings
        style.Alpha = 1.0f; // Global alpha
        style.WindowPadding = ImVec2(8, 8); // Window padding
        style.WindowRounding = 5.0f; // Window rounding
        style.FramePadding = ImVec2(5, 5); // Frame padding
        style.FrameRounding = 3.0f; // Frame rounding
        style.ItemSpacing = ImVec2(10, 10); // Item spacing
        style.ItemInnerSpacing = ImVec2(5, 5); // Inner item spacing
        style.TouchExtraPadding = ImVec2(0, 0); // Touch extra padding

        // Set colors
        ImGui::StyleColorsDark(); // Start with a dark theme (or use ImGui::StyleColorsLight())

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Text color
        colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f); // Background color
        colors[ImGuiCol_Border] = ImVec4(0.40f, 0.40f, 0.40f, 1.0f); // Border color
        colors[ImGuiCol_Button] = ImVec4(0.25f, 0.5f, 0.75f, 1.0f); // Button color
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.6f, 0.9f, 1.0f); // Button hover color
        colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.45f, 0.85f, 1.0f); // Button active color
        colors[ImGuiCol_SliderGrab] = ImVec4(0.0f, 0.7f, 0.4f, 1.0f); // Slider grab color
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.0f, 0.9f, 0.6f, 1.0f); // Slider grab active color
        colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Checkmark color
        // Add more color customizations as needed
    }
};

#endif // IM_THEME_HPP