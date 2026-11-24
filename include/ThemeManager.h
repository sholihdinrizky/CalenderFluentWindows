#pragma once

#include <windows.h>

namespace CalendarFlyout {

struct ThemeColors {
    bool isDarkMode{true};

    COLORREF background{RGB(32, 32, 32)};
    COLORREF border{RGB(55, 55, 55)};

    COLORREF textNormal{RGB(255, 255, 255)};
    COLORREF textMuted{RGB(120, 120, 120)};
    COLORREF textWeekday{RGB(165, 165, 165)};

    COLORREF sundayRed{RGB(255, 105, 105)};
    COLORREF sundayHoverBg{RGB(225, 45, 57)};
    COLORREF sundayHoverText{RGB(255, 255, 255)};

    COLORREF normalHoverBg{RGB(52, 52, 52)};

    COLORREF accentColor{RGB(0, 120, 212)};
    COLORREF accentTextColor{RGB(255, 255, 255)};

    COLORREF chevronColor{RGB(210, 210, 210)};
    COLORREF chevronHoverBg{RGB(50, 50, 50)};
};

class ThemeManager {
public:
    ThemeManager();

    void RefreshTheme();
    const ThemeColors& GetColors() const { return m_colors; }
    bool IsDarkMode() const { return m_colors.isDarkMode; }

    // Manual overrides for testing/screenshot verification
    void SetDarkModeOverride(bool isDark);
    void ClearOverride();

private:
    bool DetectWindowsDarkMode();
    COLORREF DetectWindowsAccentColor();
    static COLORREF CalculateContrastingTextColor(COLORREF bg);

    ThemeColors m_colors;
    bool m_hasOverride{false};
    bool m_overrideDark{true};
};

} // namespace CalendarFlyout
