#include "TrayIcon.h"
#include "Constants.h"
#include "Resource.h"

namespace CalendarFlyout {

TrayIcon::TrayIcon(HWND ownerHwnd, HINSTANCE instance)
    : m_ownerHwnd(ownerHwnd), m_instance(instance) {
    ZeroMemory(&m_nid, sizeof(m_nid));
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_ownerHwnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    m_nid.uCallbackMessage = Constants::WM_TRAY_CALLBACK;

    m_darkTaskbarIcon = LoadTrayIconResource(IDI_TRAY_DARK);
    m_lightTaskbarIcon = LoadTrayIconResource(IDI_TRAY_LIGHT);
    if (m_darkTaskbarIcon == nullptr) m_darkTaskbarIcon = CreateFallbackIcon(true);
    if (m_lightTaskbarIcon == nullptr) m_lightTaskbarIcon = CreateFallbackIcon(false);

    m_nid.hIcon = SelectIcon(m_taskbarDarkMode);
    wcscpy_s(m_nid.szTip, L"CalendarFlyout");
}

TrayIcon::~TrayIcon() {
    Remove();
    if (m_darkTaskbarIcon != nullptr) {
        DestroyIcon(m_darkTaskbarIcon);
        m_darkTaskbarIcon = nullptr;
    }
    if (m_lightTaskbarIcon != nullptr) {
        DestroyIcon(m_lightTaskbarIcon);
        m_lightTaskbarIcon = nullptr;
    }
}

HICON TrayIcon::LoadTrayIconResource(int resourceId) {
    // Load the icon at the small-icon metric used by the tray/notification area.
    // This keeps it crisp and avoids Windows scaling the wrong image.
    return static_cast<HICON>(LoadImageW(
        m_instance,
        MAKEINTRESOURCEW(resourceId),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR
    ));
}

HICON TrayIcon::CreateFallbackIcon(bool taskbarDarkMode) {
    // Procedural monochrome fallback if icon resources are missing.
    int cx = GetSystemMetrics(SM_CXSMICON);
    int cy = GetSystemMetrics(SM_CYSMICON);
    if (cx <= 0) cx = 16;
    if (cy <= 0) cy = 16;

    BITMAPV5HEADER bi{};
    bi.bV5Size = sizeof(BITMAPV5HEADER);
    bi.bV5Width = cx;
    bi.bV5Height = -cy; // top-down
    bi.bV5Planes = 1;
    bi.bV5BitCount = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask   = 0x00FF0000;
    bi.bV5GreenMask = 0x0000FF00;
    bi.bV5BlueMask  = 0x000000FF;
    bi.bV5AlphaMask = 0xFF000000;

    void* bits = nullptr;
    HDC hdcScreen = GetDC(nullptr);
    HBITMAP hbmColor = CreateDIBSection(hdcScreen, reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, hdcScreen);
    if (hbmColor == nullptr || bits == nullptr) {
        if (hbmColor) DeleteObject(hbmColor);
        return nullptr;
    }

    auto* px = static_cast<DWORD*>(bits);
    const DWORD color = taskbarDarkMode ? 0xFFFFFFFFu : 0xFF000000u; // ARGB in memory as BGRA via masks
    const int stroke = 1;
    const int margin = (cx <= 16) ? 1 : 2;
    const int x0 = margin;
    const int y0 = margin;
    const int x1 = cx - 1 - margin;
    const int y1 = cy - 1 - margin;
    const int headerY = y0 + (y1 - y0 + 1) * 36 / 100;

    auto setPx = [&](int x, int y) {
        if (x >= 0 && x < cx && y >= 0 && y < cy) {
            px[y * cx + x] = color;
        }
    };
    auto drawRect = [&](int rx0, int ry0, int rx1, int ry1) {
        for (int x = rx0; x <= rx1; ++x) {
            for (int t = 0; t < stroke; ++t) {
                setPx(x, ry0 + t);
                setPx(x, ry1 - t);
            }
        }
        for (int y = ry0; y <= ry1; ++y) {
            for (int t = 0; t < stroke; ++t) {
                setPx(rx0 + t, y);
                setPx(rx1 - t, y);
            }
        }
    };
    auto fillRect = [&](int rx0, int ry0, int rx1, int ry1) {
        for (int y = ry0; y <= ry1; ++y) {
            for (int x = rx0; x <= rx1; ++x) {
                setPx(x, y);
            }
        }
    };

    drawRect(x0, y0, x1, y1);
    for (int x = x0 + stroke; x <= x1 - stroke; ++x) setPx(x, headerY);

    // Simple inner blocks for legibility at 16x16.
    const int ix0 = x0 + stroke + 2;
    const int ix1 = x1 - stroke - 2;
    const int iy0 = headerY + 2;
    const int iy1 = y1 - stroke - 2;
    if (ix1 > ix0 && iy1 > iy0) {
        const int gap = 1;
        const int bw = (ix1 - ix0 + 1 - gap) / 2;
        if (bw >= 1) {
            fillRect(ix0, iy0, ix0 + bw - 1, iy1);
            fillRect(ix0 + bw + gap, iy0, ix1, iy1);
        }
    }

    HBITMAP hbmMask = CreateBitmap(cx, cy, 1, 1, nullptr);
    ICONINFO ii{};
    ii.fIcon = TRUE;
    ii.hbmColor = hbmColor;
    ii.hbmMask = hbmMask;
    HICON hIcon = CreateIconIndirect(&ii);

    DeleteObject(hbmMask);
    DeleteObject(hbmColor);
    return hIcon;
}

HICON TrayIcon::SelectIcon(bool taskbarDarkMode) const {
    // Dark taskbar needs a light (white) glyph; light taskbar needs a dark glyph.
    if (taskbarDarkMode) {
        return m_darkTaskbarIcon != nullptr ? m_darkTaskbarIcon : m_lightTaskbarIcon;
    }
    return m_lightTaskbarIcon != nullptr ? m_lightTaskbarIcon : m_darkTaskbarIcon;
}

bool TrayIcon::Add() {
    if (m_isAdded) return true;
    m_nid.hIcon = SelectIcon(m_taskbarDarkMode);
    m_isAdded = Shell_NotifyIconW(NIM_ADD, &m_nid) != FALSE;
    m_nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &m_nid);
    return m_isAdded;
}

bool TrayIcon::Remove() {
    if (!m_isAdded) return true;
    BOOL res = Shell_NotifyIconW(NIM_DELETE, &m_nid);
    m_isAdded = false;
    return res != FALSE;
}

void TrayIcon::HandleTaskbarCreated() {
    // Re-register icon when Explorer restarts
    m_isAdded = false;
    Add();
}

void TrayIcon::UpdateTheme(bool taskbarDarkMode) {
    m_taskbarDarkMode = taskbarDarkMode;
    m_nid.hIcon = SelectIcon(m_taskbarDarkMode);
    if (m_isAdded) {
        Shell_NotifyIconW(NIM_MODIFY, &m_nid);
    }
}

void TrayIcon::ShowContextMenu(POINT cursorPt) {
    HMENU hMenu = CreatePopupMenu();
    if (hMenu == nullptr) return;

    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_OPEN, L"Open Calendar");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_EXIT, L"Exit");

    // Needed for Shell_NotifyIcon popup menu to dismiss properly when clicked outside
    SetForegroundWindow(m_ownerHwnd);

    TrackPopupMenu(
        hMenu,
        TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_RIGHTALIGN,
        cursorPt.x,
        cursorPt.y,
        0,
        m_ownerHwnd,
        nullptr
    );

    DestroyMenu(hMenu);
}

} // namespace CalendarFlyout
