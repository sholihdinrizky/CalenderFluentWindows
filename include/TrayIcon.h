#pragma once

#include <windows.h>
#include <shellapi.h>

namespace CalendarFlyout {

class TrayIcon {
public:
    TrayIcon(HWND ownerHwnd, HINSTANCE instance);
    ~TrayIcon();

    bool Add();
    bool Remove();
    void HandleTaskbarCreated();
    void ShowContextMenu(POINT cursorPt, bool showOnHover, bool runAtStartup);
    void UpdateTheme(bool taskbarDarkMode);

private:
    HICON LoadTrayIconResource(int resourceId);
    HICON CreateFallbackIcon(bool taskbarDarkMode);
    HICON SelectIcon(bool taskbarDarkMode) const;

    HWND m_ownerHwnd{nullptr};
    HINSTANCE m_instance{nullptr};
    HICON m_darkTaskbarIcon{nullptr};  // white glyph for dark taskbar
    HICON m_lightTaskbarIcon{nullptr}; // dark glyph for light taskbar
    bool m_taskbarDarkMode{true};
    bool m_isAdded{false};
    NOTIFYICONDATAW m_nid{};
};

} // namespace CalendarFlyout
