#include "ThemeManager.h"
#include <dwmapi.h>

namespace CalendarFlyout {

ThemeManager::ThemeManager() {
    RefreshTheme();
}

void ThemeManager::SetDarkModeOverride(bool isDark) {
    m_hasOverride = true;
    m_overrideDark = isDark;
    RefreshTheme();
}

void ThemeManager::ClearOverride() {
    m_hasOverride = false;
    RefreshTheme();
}

void ThemeManager::RefreshTheme() {
    bool dark = m_hasOverride ? m_overrideDark : DetectWindowsDarkMode();
    m_colors.isDarkMode = dark;
    m_taskbarDarkMode = m_hasOverride ? m_overrideDark : DetectTaskbarDarkMode();

    COLORREF accent = DetectWindowsAccentColor();
    m_colors.accentColor = accent;
    m_colors.accentTextColor = CalculateContrastingTextColor(accent);

    if (dark) {
        m_colors.background = RGB(32, 32, 32);
        m_colors.border = RGB(55, 55, 55);

        m_colors.textNormal = RGB(255, 255, 255);
        m_colors.textMuted = RGB(120, 120, 120);
        m_colors.textWeekday = RGB(165, 165, 165);

        m_colors.sundayRed = RGB(255, 105, 105);
        m_colors.sundayHoverBg = RGB(225, 45, 57);
        m_colors.sundayHoverText = RGB(255, 255, 255);

        m_colors.normalHoverBg = RGB(52, 52, 52);

        m_colors.chevronColor = RGB(210, 210, 210);
        m_colors.chevronHoverBg = RGB(50, 50, 50);
    } else {
        m_colors.background = RGB(249, 249, 249);
        m_colors.border = RGB(229, 229, 229);

        m_colors.textNormal = RGB(26, 26, 26);
        m_colors.textMuted = RGB(160, 160, 160);
        m_colors.textWeekday = RGB(100, 100, 100);

        m_colors.sundayRed = RGB(209, 52, 56);
        m_colors.sundayHoverBg = RGB(209, 52, 56);
        m_colors.sundayHoverText = RGB(255, 255, 255);

        m_colors.normalHoverBg = RGB(232, 232, 232);

        m_colors.chevronColor = RGB(90, 90, 90);
        m_colors.chevronHoverBg = RGB(225, 225, 225);
    }
}

bool ThemeManager::DetectWindowsDarkMode() {
    HKEY hKey;
    if (RegOpenKeyExW(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            0,
            KEY_READ,
            &hKey) == ERROR_SUCCESS) {
        DWORD value = 1;
        DWORD size = sizeof(value);
        if (RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return (value == 0); // 0 = dark mode, 1 = light mode
        }
        RegCloseKey(hKey);
    }
    return true; // Default to dark mode on Windows 11
}

bool ThemeManager::DetectTaskbarDarkMode() {
    // Windows 11 taskbar appearance is controlled by SystemUsesLightTheme.
    // 0 = dark taskbar/system surfaces, 1 = light taskbar/system surfaces.
    // We read this separately from AppsUseLightTheme because the app theme and
    // taskbar theme can be configured independently in Windows settings.
    HKEY hKey;
    if (RegOpenKeyExW(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            0,
            KEY_READ,
            &hKey) == ERROR_SUCCESS) {
        DWORD value = 0;
        DWORD size = sizeof(value);
        if (RegQueryValueExW(hKey, L"SystemUsesLightTheme", nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return (value == 0);
        }
        RegCloseKey(hKey);
    }

    // Fallback: if SystemUsesLightTheme is unavailable, use the app theme as a
    // best-effort approximation rather than guessing blindly.
    return DetectWindowsDarkMode();
}

COLORREF ThemeManager::DetectWindowsAccentColor() {
    // 1. Try reading registry AccentColor from DWM
    HKEY hKey;
    if (RegOpenKeyExW(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\DWM",
            0,
            KEY_READ,
            &hKey) == ERROR_SUCCESS) {
        DWORD rawColor = 0;
        DWORD size = sizeof(rawColor);
        if (RegQueryValueExW(hKey, L"AccentColor", nullptr, nullptr, reinterpret_cast<LPBYTE>(&rawColor), &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            // Registry format is ABGR (0xAABBGGRR)
            BYTE r = static_cast<BYTE>(rawColor & 0xFF);
            BYTE g = static_cast<BYTE>((rawColor >> 8) & 0xFF);
            BYTE b = static_cast<BYTE>((rawColor >> 16) & 0xFF);
            return RGB(r, g, b);
        }
        RegCloseKey(hKey);
    }

    // 2. Fallback to DwmGetColorizationColor
    DWORD colorization = 0;
    BOOL opaque = FALSE;
    if (SUCCEEDED(DwmGetColorizationColor(&colorization, &opaque))) {
        // Format is ARGB
        BYTE r = static_cast<BYTE>((colorization >> 16) & 0xFF);
        BYTE g = static_cast<BYTE>((colorization >> 8) & 0xFF);
        BYTE b = static_cast<BYTE>(colorization & 0xFF);
        return RGB(r, g, b);
    }

    // 3. Standard Windows 11 Fluent Blue fallback
    return RGB(0, 120, 212);
}

COLORREF ThemeManager::CalculateContrastingTextColor(COLORREF bg) {
    BYTE r = GetRValue(bg);
    BYTE g = GetGValue(bg);
    BYTE b = GetBValue(bg);
    // Standard relative luminance formula
    double luminance = (0.299 * r + 0.587 * g + 0.114 * b) / 255.0;
    if (luminance > 0.55) {
        return RGB(20, 20, 20); // Dark text on light accent
    }
    return RGB(255, 255, 255);  // White text on dark accent
}

} // namespace CalendarFlyout
