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

    m_icon = LoadAppIcon();
    m_nid.hIcon = m_icon;

    wcscpy_s(m_nid.szTip, L"CalendarFlyout");
}

TrayIcon::~TrayIcon() {
    Remove();
    if (m_icon != nullptr) {
        DestroyIcon(m_icon);
        m_icon = nullptr;
    }
}

HICON TrayIcon::LoadAppIcon() {
    // Try to load embedded icon first
    HICON hIcon = (HICON)LoadImageW(
        m_instance,
        MAKEINTRESOURCEW(IDI_APP_ICON),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR
    );

    if (hIcon != nullptr) {
        return hIcon;
    }

    // Procedural fallback icon if resource is somehow missing
    int cx = GetSystemMetrics(SM_CXSMICON);
    int cy = GetSystemMetrics(SM_CYSMICON);
    if (cx <= 0) cx = 16;
    if (cy <= 0) cy = 16;

    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hbmColor = CreateCompatibleBitmap(hdcScreen, cx, cy);
    HBITMAP hbmMask = CreateBitmap(cx, cy, 1, 1, nullptr);

    HBITMAP hOldBm = (HBITMAP)SelectObject(hdcMem, hbmColor);
    RECT rc{0, 0, cx, cy};
    HBRUSH hBrBg = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hdcMem, &rc, hBrBg);
    DeleteObject(hBrBg);

    RECT rcHeader{0, 0, cx, cy / 3};
    HBRUSH hBrRed = CreateSolidBrush(RGB(225, 45, 57));
    FillRect(hdcMem, &rcHeader, hBrRed);
    DeleteObject(hBrRed);

    SelectObject(hdcMem, hOldBm);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);

    ICONINFO ii{};
    ii.fIcon = TRUE;
    ii.hbmColor = hbmColor;
    ii.hbmMask = hbmMask;
    hIcon = CreateIconIndirect(&ii);

    DeleteObject(hbmColor);
    DeleteObject(hbmMask);

    return hIcon;
}

bool TrayIcon::Add() {
    if (m_isAdded) return true;
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
