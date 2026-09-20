#include "ui/Theme.hpp"
#include <algorithm>

namespace Ps4Atlus {

ImVec4 ThemeManager::PrimaryColor = ImVec4(0.90f, 0.07f, 0.15f, 1.0f);
ImVec4 ThemeManager::PrimaryHoverColor = ImVec4(1.00f, 0.20f, 0.25f, 1.0f);
ImVec4 ThemeManager::PrimaryActiveColor = ImVec4(0.75f, 0.05f, 0.12f, 1.0f);
ImVec4 ThemeManager::AccentColor = ImVec4(1.00f, 0.25f, 0.25f, 1.0f);
ImVec4 ThemeManager::SuccessColor = ImVec4(0.18f, 0.80f, 0.44f, 1.0f);
ImVec4 ThemeManager::WarningColor = ImVec4(0.95f, 0.65f, 0.10f, 1.0f);
ImVec4 ThemeManager::ErrorColor = ImVec4(0.92f, 0.22f, 0.22f, 1.0f);
ImVec4 ThemeManager::CardBgColor = ImVec4(0.12f, 0.13f, 0.17f, 0.90f);
ImVec4 ThemeManager::CardBorderColor = ImVec4(0.24f, 0.26f, 0.33f, 0.60f);
ImVec4 ThemeManager::SubtleTextColor = ImVec4(0.65f, 0.68f, 0.75f, 1.0f);

void ThemeManager::InitializeStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    style.WindowRounding = 10.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;

    style.WindowPadding = ImVec2(14.0f, 14.0f);
    style.FramePadding = ImVec2(10.0f, 6.0f);
    style.ItemSpacing = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.ScrollbarSize = 12.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;

    // Dark sleek base palette
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                  = ImVec4(0.96f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.48f, 0.50f, 0.56f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.08f, 0.09f, 0.11f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.11f, 0.12f, 0.15f, 0.70f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.12f, 0.13f, 0.17f, 0.98f);
    colors[ImGuiCol_Border]                = ImVec4(0.22f, 0.24f, 0.30f, 0.80f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.15f, 0.16f, 0.21f, 0.80f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.20f, 0.22f, 0.29f, 0.90f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.25f, 0.27f, 0.36f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.07f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.09f, 0.10f, 0.13f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.07f, 0.08f, 0.10f, 0.75f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.10f, 0.11f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.08f, 0.09f, 0.11f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.25f, 0.27f, 0.34f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.35f, 0.37f, 0.46f, 0.90f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.45f, 0.47f, 0.58f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.95f, 0.95f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.35f, 0.37f, 0.46f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.48f, 0.50f, 0.62f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.20f, 0.22f, 0.28f, 0.70f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.30f, 0.33f, 0.42f, 0.80f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.40f, 0.44f, 0.55f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.20f, 0.22f, 0.28f, 0.40f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.35f, 0.38f, 0.48f, 0.70f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.50f, 0.54f, 0.68f, 0.90f);
    colors[ImGuiCol_Header]                = ImVec4(0.18f, 0.20f, 0.26f, 0.80f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.25f, 0.27f, 0.36f, 0.90f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.30f, 0.33f, 0.44f, 1.00f);
}

void ThemeManager::ApplyGameTheme(const GameProfile& profile) {
    ImGuiStyle& style = ImGui::GetStyle();
    
    PrimaryColor = ImVec4(profile.primaryColor[0], profile.primaryColor[1], profile.primaryColor[2], profile.primaryColor[3]);
    AccentColor = ImVec4(profile.accentColor[0], profile.accentColor[1], profile.accentColor[2], profile.accentColor[3]);
    
    PrimaryHoverColor = ImVec4(
        std::min(PrimaryColor.x * 1.25f, 1.0f),
        std::min(PrimaryColor.y * 1.25f, 1.0f),
        std::min(PrimaryColor.z * 1.25f, 1.0f),
        1.0f
    );
    
    PrimaryActiveColor = ImVec4(
        PrimaryColor.x * 0.8f,
        PrimaryColor.y * 0.8f,
        PrimaryColor.z * 0.8f,
        1.0f
    );

    // Apply button and tab accents
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Button]         = ImVec4(PrimaryColor.x * 0.75f, PrimaryColor.y * 0.75f, PrimaryColor.z * 0.75f, 0.85f);
    colors[ImGuiCol_ButtonHovered]  = PrimaryHoverColor;
    colors[ImGuiCol_ButtonActive]   = PrimaryActiveColor;

    colors[ImGuiCol_Tab]                = ImVec4(0.14f, 0.15f, 0.19f, 0.90f);
    colors[ImGuiCol_TabHovered]         = ImVec4(PrimaryColor.x * 0.5f, PrimaryColor.y * 0.5f, PrimaryColor.z * 0.5f, 0.80f);
    colors[ImGuiCol_TabSelected]        = ImVec4(PrimaryColor.x * 0.85f, PrimaryColor.y * 0.85f, PrimaryColor.z * 0.85f, 1.00f);
    colors[ImGuiCol_TabDimmed]          = ImVec4(0.12f, 0.13f, 0.16f, 0.70f);
    colors[ImGuiCol_TabDimmedSelected]  = ImVec4(PrimaryColor.x * 0.6f, PrimaryColor.y * 0.6f, PrimaryColor.z * 0.6f, 0.80f);

    colors[ImGuiCol_Header]         = ImVec4(PrimaryColor.x * 0.40f, PrimaryColor.y * 0.40f, PrimaryColor.z * 0.40f, 0.70f);
    colors[ImGuiCol_HeaderHovered]  = ImVec4(PrimaryColor.x * 0.60f, PrimaryColor.y * 0.60f, PrimaryColor.z * 0.60f, 0.80f);
    colors[ImGuiCol_HeaderActive]   = ImVec4(PrimaryColor.x * 0.70f, PrimaryColor.y * 0.70f, PrimaryColor.z * 0.70f, 0.90f);

    colors[ImGuiCol_CheckMark]      = AccentColor;
    colors[ImGuiCol_SliderGrab]     = PrimaryColor;
    colors[ImGuiCol_SliderGrabActive] = AccentColor;
}

void ThemeManager::RenderHeaderBadge(const char* label, const ImVec4& color, const ImVec4& textColor) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 textSize = ImGui::CalcTextSize(label);
    ImVec2 padding(8.0f, 4.0f);
    ImVec2 min = pos;
    ImVec2 max = ImVec2(pos.x + textSize.x + padding.x * 2.0f, pos.y + textSize.y + padding.y * 2.0f);

    drawList->AddRectFilled(min, max, ImGui::GetColorU32(color), 5.0f);
    drawList->AddText(ImVec2(min.x + padding.x, min.y + padding.y), ImGui::GetColorU32(textColor), label);

    ImGui::Dummy(ImVec2(textSize.x + padding.x * 2.0f, textSize.y + padding.y * 2.0f));
}

void ThemeManager::RenderStatusPill(const char* label, bool isOk) {
    ImVec4 bg = isOk ? ImVec4(0.12f, 0.45f, 0.25f, 0.90f) : ImVec4(0.55f, 0.18f, 0.18f, 0.90f);
    ImVec4 border = isOk ? SuccessColor : ErrorColor;
    ImVec4 text = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 textSize = ImGui::CalcTextSize(label);
    ImVec2 padding(8.0f, 3.0f);
    ImVec2 min = pos;
    ImVec2 max = ImVec2(pos.x + textSize.x + padding.x * 2.0f, pos.y + textSize.y + padding.y * 2.0f);

    drawList->AddRectFilled(min, max, ImGui::GetColorU32(bg), 12.0f);
    drawList->AddRect(min, max, ImGui::GetColorU32(border), 12.0f, 0, 1.2f);
    drawList->AddText(ImVec2(min.x + padding.x, min.y + padding.y), ImGui::GetColorU32(text), label);

    ImGui::Dummy(ImVec2(textSize.x + padding.x * 2.0f, textSize.y + padding.y * 2.0f));
}

} // namespace Ps4Atlus
