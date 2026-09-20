#pragma once

#include "models/GameProfile.hpp"
#include <imgui.h>

namespace Ps4Atlus {

class ThemeManager {
public:
    static void InitializeStyle();
    static void ApplyGameTheme(const GameProfile& profile);

    static ImVec4 PrimaryColor;
    static ImVec4 PrimaryHoverColor;
    static ImVec4 PrimaryActiveColor;
    static ImVec4 AccentColor;
    static ImVec4 SuccessColor;
    static ImVec4 WarningColor;
    static ImVec4 ErrorColor;
    static ImVec4 CardBgColor;
    static ImVec4 CardBorderColor;
    static ImVec4 SubtleTextColor;

    static void RenderHeaderBadge(const char* label, const ImVec4& color, const ImVec4& textColor = ImVec4(1,1,1,1));
    static void RenderStatusPill(const char* label, bool isOk);
};

} // namespace Ps4Atlus
