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
    void ShowContextMenu(POINT cursorPt);

private:
    HICON LoadAppIcon();

    HWND m_ownerHwnd{nullptr};
    HINSTANCE m_instance{nullptr};
    HICON m_icon{nullptr};
    bool m_isAdded{false};
    NOTIFYICONDATAW m_nid{};
};

} // namespace CalendarFlyout
