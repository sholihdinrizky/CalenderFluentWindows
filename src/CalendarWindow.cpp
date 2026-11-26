#include "CalendarWindow.h"
#include "Constants.h"
#include <dwmapi.h>
#include <fstream>
#include <vector>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif

namespace CalendarFlyout {

namespace {
    constexpr const wchar_t* POPUP_CLASS_NAME = L"CalendarFlyout_PopupWindow";

    UINT GetWindowDpi(HWND hwnd) {
        HMODULE hUser = GetModuleHandleW(L"user32.dll");
        if (hUser) {
            using GetDpiFn = UINT(WINAPI*)(HWND);
            auto pfn = reinterpret_cast<GetDpiFn>(reinterpret_cast<void*>(GetProcAddress(hUser, "GetDpiForWindow")));
            if (pfn && hwnd) {
                UINT dpi = pfn(hwnd);
                if (dpi > 0) return dpi;
            }
        }
        HDC hdc = GetDC(nullptr);
        int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(nullptr, hdc);
        return (dpi > 0) ? dpi : 96;
    }
}

CalendarWindow::CalendarWindow(HINSTANCE hInstance)
    : m_hInstance(hInstance) {
}

CalendarWindow::~CalendarWindow() {
    if (m_hwnd != nullptr) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    UnregisterClassW(POPUP_CLASS_NAME, m_hInstance);
}

bool CalendarWindow::Create() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    wc.hbrBackground = nullptr; // Handled in double-buffered WM_PAINT
    wc.lpszClassName = POPUP_CLASS_NAME;

    RegisterClassExW(&wc);

    m_currentDpi = GetWindowDpi(nullptr);
    int width = MulDiv(340, m_currentDpi, 96);
    m_renderer.UpdateDpi(m_currentDpi, width, 0);
    int height = m_renderer.GetMetrics().computedHeight;

    // WS_EX_TOOLWINDOW prevents showing up in the taskbar and Alt+Tab
    // WS_EX_TOPMOST keeps it on top of other desktop windows when active
    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        POPUP_CLASS_NAME,
        L"CalendarFlyout",
        WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        0, 0, width, height,
        nullptr,
        nullptr,
        m_hInstance,
        this
    );

    if (m_hwnd == nullptr) {
        return false;
    }

    UpdateDwmAttributes();
    m_renderer.UpdateDpi(m_currentDpi, width, height);

    return true;
}

void CalendarWindow::UpdateDwmAttributes() {
    if (!m_hwnd) return;

    // 1. Windows 11 rounded corners
    DWORD cornerPref = DWMWCP_ROUND;
    DwmSetWindowAttribute(m_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPref, sizeof(cornerPref));

    // 2. Immersive dark mode attribute
    BOOL isDark = m_themeManager.IsDarkMode() ? TRUE : FALSE;
    DwmSetWindowAttribute(m_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &isDark, sizeof(isDark));
}

void CalendarWindow::RecalculateSizeAndDpi() {
    m_currentDpi = GetWindowDpi(m_hwnd);
    int width = MulDiv(340, m_currentDpi, 96);
    m_renderer.UpdateDpi(m_currentDpi, width, 0);
    int height = m_renderer.GetMetrics().computedHeight;
    SetWindowPos(m_hwnd, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void CalendarWindow::PositionNearTray(POINT trayPt) {
    RecalculateSizeAndDpi();

    RECT rcWin;
    GetWindowRect(m_hwnd, &rcWin);
    int winW = rcWin.right - rcWin.left;
    int winH = rcWin.bottom - rcWin.top;

    // Determine target monitor work area
    HMONITOR hMon = MonitorFromPoint(trayPt, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi{};
    mi.cbSize = sizeof(MONITORINFO);
    GetMonitorInfoW(hMon, &mi);

    RECT rcWork = mi.rcWork;

    int posX = 0;
    int posY = 0;

    if (trayPt.x != 0 || trayPt.y != 0) {
        // Place near the tray icon coordinates
        posX = trayPt.x - winW / 2;
        posY = trayPt.y - winH - MulDiv(12, m_currentDpi, 96);
    } else {
        // Default: Bottom-right corner inside the work area
        posX = rcWork.right - winW - MulDiv(12, m_currentDpi, 96);
        posY = rcWork.bottom - winH - MulDiv(12, m_currentDpi, 96);
    }

    // Ensure within monitor work area bounds
    if (posX + winW > rcWork.right) posX = rcWork.right - winW - MulDiv(8, m_currentDpi, 96);
    if (posX < rcWork.left) posX = rcWork.left + MulDiv(8, m_currentDpi, 96);
    if (posY + winH > rcWork.bottom) posY = rcWork.bottom - winH - MulDiv(8, m_currentDpi, 96);
    if (posY < rcWork.top) posY = rcWork.top + MulDiv(8, m_currentDpi, 96);

    // Position window off-screen/silently without SWP_SHOWWINDOW
    // so AnimateWindow can smoothly slide it into view without visual jumps
    SetWindowPos(m_hwnd, HWND_TOPMOST, posX, posY, winW, winH, SWP_NOACTIVATE);
}

void CalendarWindow::Show(bool show, PopupOpenSource source) {
    if (show) {
        if (IsVisible()) {
            // If already visible, only upgrade hover-open to click-open.
            if (source == PopupOpenSource::Click) {
                m_openSource = PopupOpenSource::Click;
            }
            return;
        }
        m_openSource = source;
        m_isPointerInside = false;
        m_themeManager.RefreshTheme();
        m_dateService.ResetToToday();
        UpdateDwmAttributes();
        PositionNearTray();

        // Paint the full calendar content before making the window visible.
        // This avoids the "thin outline / blank popup" symptom: the user sees
        // a fully rendered calendar immediately instead of an empty shell.
        InvalidateRect(m_hwnd, nullptr, FALSE);
        ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
        UpdateWindow(m_hwnd); // Synchronously dispatch WM_PAINT now.
        SetForegroundWindow(m_hwnd);
        SetFocus(m_hwnd);
    } else {
        if (!IsVisible()) return;
        m_hoveredCellIndex = -1;
        m_hoveredPrev = false;
        m_hoveredNext = false;
        m_isMouseTracking = false;
        m_isPointerInside = false;
        m_openSource = PopupOpenSource::None;
        // Hide immediately. Responsiveness is more important than a close animation,
        // and AnimateWindow can block the message pump on some popup/DWM configurations.
        ShowWindow(m_hwnd, SW_HIDE);
    }
}

bool CalendarWindow::IsVisible() const {
    return (m_hwnd != nullptr) && (IsWindowVisible(m_hwnd) != FALSE);
}

void CalendarWindow::Refresh() {
    if (m_hwnd) {
        InvalidateRect(m_hwnd, nullptr, FALSE);
        UpdateWindow(m_hwnd);
    }
}

void CalendarWindow::ApplyThemeChange() {
    m_themeManager.RefreshTheme();
    UpdateDwmAttributes();
    Refresh();
}

void CalendarWindow::SetHoverCellForTest(int cellIndex) {
    m_hoveredCellIndex = cellIndex;
    Refresh();
}

void CalendarWindow::CaptureToFile(const std::wstring& outPath) {
    if (!m_hwnd) return;

    RECT rc;
    GetClientRect(m_hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    HDC hdcScreen = GetDC(m_hwnd);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
    HGDIOBJ oldBm = SelectObject(hdcMem, hBitmap);

    // Render directly into memory DC
    m_renderer.Render(
        hdcMem,
        rc,
        m_dateService,
        m_themeManager.GetColors(),
        m_hoveredCellIndex,
        m_hoveredPrev,
        m_hoveredNext
    );

    // Save as 24-bit uncompressed BMP file
    BITMAPINFOHEADER bih{};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = height; // bottom-up
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = BI_RGB;

    int rowStride = ((width * 3 + 3) / 4) * 4;
    DWORD imageSize = rowStride * height;
    bih.biSizeImage = imageSize;

    std::vector<BYTE> pixels(imageSize, 0);
    GetDIBits(hdcMem, hBitmap, 0, height, pixels.data(), reinterpret_cast<BITMAPINFO*>(&bih), DIB_RGB_COLORS);

    BITMAPFILEHEADER bfh{};
    bfh.bfType = 0x4D42; // "BM"
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + imageSize;

    std::ofstream file(outPath.c_str(), std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(&bfh), sizeof(bfh));
        file.write(reinterpret_cast<const char*>(&bih), sizeof(bih));
        file.write(reinterpret_cast<const char*>(pixels.data()), imageSize);
    }

    SelectObject(hdcMem, oldBm);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(m_hwnd, hdcScreen);
}

LRESULT CALLBACK CalendarWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    CalendarWindow* pThis = nullptr;
    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = static_cast<CalendarWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        pThis = reinterpret_cast<CalendarWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (pThis != nullptr) {
        return pThis->HandleMessage(hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CalendarWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) {
            m_lastDismissTime = GetTickCount64();
            Show(false);
        } else {
            SetFocus(hwnd);
        }
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            Show(false);
            return 0;
        }
        break;

    case WM_MOUSEMOVE: {
        m_isPointerInside = true;
        if (!m_isMouseTracking) {
            TRACKMOUSEEVENT tme{};
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            if (TrackMouseEvent(&tme)) {
                m_isMouseTracking = true;
            }
        }

        POINT pt = { static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam)) };
        int cellIndex = m_renderer.HitTestCell(pt);
        bool prevHover = m_renderer.HitTestPrev(pt);
        bool nextHover = m_renderer.HitTestNext(pt);

        // Repaint ONLY when the hovered target actually changes
        if (cellIndex != m_hoveredCellIndex || prevHover != m_hoveredPrev || nextHover != m_hoveredNext) {
            m_hoveredCellIndex = cellIndex;
            m_hoveredPrev = prevHover;
            m_hoveredNext = nextHover;
            InvalidateRect(hwnd, nullptr, FALSE);
            UpdateWindow(hwnd);
        }
        return 0;
    }

    case WM_MOUSELEAVE: {
        m_isMouseTracking = false;
        m_isPointerInside = false;
        if (m_hoveredCellIndex != -1 || m_hoveredPrev || m_hoveredNext) {
            m_hoveredCellIndex = -1;
            m_hoveredPrev = false;
            m_hoveredNext = false;
            InvalidateRect(hwnd, nullptr, FALSE);
            UpdateWindow(hwnd);
        }
        // Notify the controller so hover-opened popups can dismiss when the
        // pointer leaves both the popup and the tray icon.
        if (m_controllerHwnd) {
            PostMessageW(m_controllerHwnd, Constants::WM_POPUP_POINTER_LEFT, 0, 0);
        }
        return 0;
    }

    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK: {
        // Clicking inside a hover-opened popup should make it behave like a
        // click-opened popup: it stays open until explicit dismissal.
        if (m_openSource == PopupOpenSource::Hover) {
            m_openSource = PopupOpenSource::Click;
        }
        POINT pt = { static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam)) };
        if (m_renderer.HitTestPrev(pt)) {
            m_dateService.NavigatePreviousMonth();
            m_hoveredCellIndex = m_renderer.HitTestCell(pt);
            InvalidateRect(hwnd, nullptr, FALSE);
            UpdateWindow(hwnd);
            return 0;
        }
        if (m_renderer.HitTestNext(pt)) {
            m_dateService.NavigateNextMonth();
            m_hoveredCellIndex = m_renderer.HitTestCell(pt);
            InvalidateRect(hwnd, nullptr, FALSE);
            UpdateWindow(hwnd);
            return 0;
        }
        return 0;
    }

    case WM_SETCURSOR: {
        if (LOWORD(lParam) == HTCLIENT) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);
            if (m_renderer.HitTestPrev(pt) || m_renderer.HitTestNext(pt)) {
                SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32649))); // IDC_HAND
                return TRUE;
            }
        }
        break;
    }

    case WM_PAINT:
        OnPaint(hwnd);
        return 0;

    case WM_ERASEBKGND:
        // Handled completely by double-buffered WM_PAINT to prevent flicker
        return 1;

    case WM_DPICHANGED: {
        m_currentDpi = HIWORD(wParam);
        auto prc = reinterpret_cast<RECT*>(lParam);
        SetWindowPos(hwnd, nullptr, prc->left, prc->top, prc->right - prc->left, prc->bottom - prc->top, SWP_NOZORDER | SWP_NOACTIVATE);
        m_renderer.UpdateDpi(m_currentDpi, prc->right - prc->left, prc->bottom - prc->top);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void CalendarWindow::OnPaint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT rcClient;
    GetClientRect(hwnd, &rcClient);

    m_renderer.Render(
        hdc,
        rcClient,
        m_dateService,
        m_themeManager.GetColors(),
        m_hoveredCellIndex,
        m_hoveredPrev,
        m_hoveredNext
    );

    EndPaint(hwnd, &ps);
}

} // namespace CalendarFlyout
