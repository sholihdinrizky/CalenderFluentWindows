#include "Application.h"
#include "Constants.h"
#include "Resource.h"
#include <string>

namespace CalendarFlyout {

Application::Application(HINSTANCE hInstance)
    : m_hInstance(hInstance) {
    m_taskbarCreatedMsg = RegisterWindowMessageW(L"TaskbarCreated");
    m_activateInstanceMsg = RegisterWindowMessageW(Constants::ACTIVATE_MSG_NAME);
}

Application::~Application() {
    if (m_hwndController) {
        KillTimer(m_hwndController, Constants::TIMER_TRAY_HOVER_OPEN);
        KillTimer(m_hwndController, Constants::TIMER_HOVER_DISMISS);
    }
    if (m_calendarWindow) {
        m_calendarWindow->Show(false);
    }
    if (m_trayIcon) {
        m_trayIcon->Remove();
    }
    if (m_hwndController) {
        DestroyWindow(m_hwndController);
        m_hwndController = nullptr;
    }
    UnregisterClassW(Constants::CONTROLLER_CLASS_NAME, m_hInstance);
}

bool Application::Initialize() {
    // Register controller window class
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = m_hInstance;
    wc.lpszClassName = Constants::CONTROLLER_CLASS_NAME;

    if (!RegisterClassExW(&wc)) {
        return false;
    }

    // Create a hidden top-level controller window
    m_hwndController = CreateWindowExW(
        0,
        Constants::CONTROLLER_CLASS_NAME,
        L"CalendarFlyout_Controller",
        WS_POPUP,
        0, 0, 0, 0,
        nullptr,
        nullptr,
        m_hInstance,
        this
    );

    if (m_hwndController == nullptr) {
        return false;
    }

    // Initialize calendar popup window
    m_calendarWindow = std::make_unique<CalendarWindow>(m_hInstance);
    if (!m_calendarWindow->Create()) {
        return false;
    }
    m_calendarWindow->SetControllerHwnd(m_hwndController);

    // Initialize and show tray icon
    m_trayIcon = std::make_unique<TrayIcon>(m_hwndController, m_hInstance);
    m_trayIcon->UpdateTheme(m_calendarWindow->GetThemeManager().IsTaskbarDarkMode());
    m_trayIcon->Add();

    InitializeSettings();

    return true;
}

int Application::Run() {
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK Application::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Application* pApp = nullptr;

    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pApp = static_cast<Application*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pApp));
    } else {
        pApp = reinterpret_cast<Application*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (pApp != nullptr) {
        return pApp->HandleMessage(hwnd, msg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT Application::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == m_taskbarCreatedMsg && m_taskbarCreatedMsg != 0) {
        if (m_trayIcon) {
            m_trayIcon->HandleTaskbarCreated();
            if (m_calendarWindow) {
                m_trayIcon->UpdateTheme(m_calendarWindow->GetThemeManager().IsTaskbarDarkMode());
            }
        }
        return 0;
    }

    if (msg == m_activateInstanceMsg && m_activateInstanceMsg != 0) {
        OnActivateInstance();
        return 0;
    }

    switch (msg) {
    case WM_SETTINGCHANGE:
    case WM_THEMECHANGED:
    case WM_SYSCOLORCHANGE:
        if (m_calendarWindow) {
            auto& theme = m_calendarWindow->GetThemeManager();
            const bool wasTaskbarDark = theme.IsTaskbarDarkMode();
            m_calendarWindow->ApplyThemeChange();
            if (m_trayIcon && wasTaskbarDark != theme.IsTaskbarDarkMode()) {
                m_trayIcon->UpdateTheme(theme.IsTaskbarDarkMode());
            }
        }
        return 0;
    case Constants::WM_TRAY_CALLBACK:
        OnTrayNotification(wParam, lParam);
        return 0;

    case WM_TIMER:
        if (wParam == Constants::TIMER_TRAY_HOVER_OPEN) {
            CancelTrayHoverTimer();
            // Open only if the popup is still hidden. This avoids re-opening
            // immediately after a focus-loss dismissal or interrupting a visible popup.
            if (m_calendarWindow && !m_calendarWindow->IsVisible()) {
                m_calendarWindow->Show(true, PopupOpenSource::Hover);
                UpdateHoverDismissState();
            }
            return 0;
        }
        if (wParam == Constants::TIMER_HOVER_DISMISS) {
            CancelHoverDismissTimer();
            // Grace period elapsed: if pointer is still outside both tray icon and
            // hover-opened popup, close it.
            if (m_calendarWindow && m_calendarWindow->IsVisible() &&
                m_calendarWindow->GetOpenSource() == PopupOpenSource::Hover) {
                POINT pt{};
                GetCursorPos(&pt);
                HWND hwndUnderCursor = WindowFromPoint(pt);
                const bool inPopup = (hwndUnderCursor == m_calendarWindow->GetHwnd()) ||
                                     (hwndUnderCursor != nullptr && IsChild(m_calendarWindow->GetHwnd(), hwndUnderCursor));
                if (!m_isPointerOverTrayIcon && !inPopup) {
                    m_calendarWindow->Show(false);
                }
            }
            return 0;
        }
        break;

    case Constants::WM_POPUP_POINTER_LEFT:
        UpdateHoverDismissState();
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_TRAY_OPEN:
            if (m_calendarWindow) {
                CancelHoverDismissTimer();
                m_calendarWindow->Show(true, PopupOpenSource::Click);
            }
            break;
        case IDM_TRAY_HOVER:
            SetShowOnHoverEnabled(!m_showOnHover);
            break;
        case IDM_TRAY_STARTUP:
            SetRunAtStartupEnabled(!IsRunAtStartupEnabled());
            break;
        case IDM_TRAY_EXIT:
            if (m_calendarWindow) {
                m_calendarWindow->Show(false);
            }
            if (m_trayIcon) {
                m_trayIcon->Remove();
            }
            PostQuitMessage(0);
            break;
        }
        return 0;

    case WM_DESTROY:
        if (m_calendarWindow) {
            m_calendarWindow->Show(false);
        }
        if (m_trayIcon) {
            m_trayIcon->Remove();
        }
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void Application::OnTrayNotification(WPARAM /*wParam*/, LPARAM lParam) {
    UINT event = LOWORD(lParam);
    switch (event) {
    case NIN_SELECT:
    case NIN_KEYSELECT:
    case WM_LBUTTONUP:
        m_isPointerOverTrayIcon = true;
        CancelTrayHoverTimer();
        CancelHoverDismissTimer();
        ToggleCalendar();
        break;

    case WM_RBUTTONUP:
    case WM_CONTEXTMENU:
        m_isPointerOverTrayIcon = true;
        CancelTrayHoverTimer();
        CancelHoverDismissTimer();
        if (m_calendarWindow && m_calendarWindow->IsVisible()) {
            m_calendarWindow->Show(false);
        }
        {
            POINT pt;
            GetCursorPos(&pt);
            if (m_trayIcon) {
                m_trayIcon->ShowContextMenu(pt, m_showOnHover, IsRunAtStartupEnabled());
            }
        }
        break;

    case NIN_POPUPOPEN:
        // Received when the user hovers the tray icon (NOTIFYICON_VERSION_4).
        // Use a short timer so passing over the icon does not open the popup.
        m_isPointerOverTrayIcon = true;
        CancelHoverDismissTimer();
        if (!m_showOnHover) {
            break;
        }
        if (m_calendarWindow && m_calendarWindow->IsVisible()) {
            // Already visible (click-opened or hover-opened); keep as-is.
            break;
        }
        if (m_calendarWindow) {
            const ULONGLONG now = GetTickCount64();
            if (now - m_calendarWindow->GetLastDismissTime() < 250) {
                break;
            }
        }
        StartTrayHoverTimer();
        break;

    case NIN_POPUPCLOSE:
        // Received when hover leaves the tray icon or the popup is dismissed.
        m_isPointerOverTrayIcon = false;
        CancelTrayHoverTimer();
        UpdateHoverDismissState();
        break;
    }
}

void Application::StartTrayHoverTimer() {
    if (m_hwndController == nullptr) return;
    if (m_calendarWindow && m_calendarWindow->IsVisible()) return;
    m_trayHoverTimerActive = (SetTimer(
        m_hwndController,
        Constants::TIMER_TRAY_HOVER_OPEN,
        Constants::TRAY_HOVER_OPEN_DELAY_MS,
        nullptr
    ) != 0);
}

void Application::CancelTrayHoverTimer() {
    if (!m_trayHoverTimerActive || m_hwndController == nullptr) return;
    KillTimer(m_hwndController, Constants::TIMER_TRAY_HOVER_OPEN);
    m_trayHoverTimerActive = false;
}

void Application::StartHoverDismissTimer() {
    if (m_hoverDismissTimerActive || m_hwndController == nullptr) return;
    m_hoverDismissTimerActive = (SetTimer(
        m_hwndController,
        Constants::TIMER_HOVER_DISMISS,
        Constants::HOVER_DISMISS_DELAY_MS,
        nullptr
    ) != 0);
}

void Application::CancelHoverDismissTimer() {
    if (!m_hoverDismissTimerActive || m_hwndController == nullptr) return;
    KillTimer(m_hwndController, Constants::TIMER_HOVER_DISMISS);
    m_hoverDismissTimerActive = false;
}

void Application::UpdateHoverDismissState() {
    if (!m_calendarWindow || !m_calendarWindow->IsVisible()) {
        CancelHoverDismissTimer();
        return;
    }

    // Only hover-opened calendars auto-dismiss when pointer leaves both areas.
    if (m_calendarWindow->GetOpenSource() != PopupOpenSource::Hover) {
        CancelHoverDismissTimer();
        return;
    }

    POINT pt{};
    GetCursorPos(&pt);
    HWND hwndUnderCursor = WindowFromPoint(pt);
    const bool inPopup = (hwndUnderCursor == m_calendarWindow->GetHwnd()) ||
                         (hwndUnderCursor != nullptr && IsChild(m_calendarWindow->GetHwnd(), hwndUnderCursor));

    // Keep open while pointer is over tray icon or popup.
    if (m_isPointerOverTrayIcon || inPopup) {
        CancelHoverDismissTimer();
        return;
    }

    // Pointer left both areas: start grace timer. Actual close happens in
    // WM_TIMER after the grace period, preventing tray->popup transition flicker.
    StartHoverDismissTimer();
    if (!m_hoverDismissTimerActive) {
        // Fail-safe if timer cannot be created.
        m_calendarWindow->Show(false);
    }
}

void Application::OnActivateInstance() {
    // When a second instance is launched, activate/show the calendar popup
    if (m_calendarWindow && !m_calendarWindow->IsVisible()) {
        CancelHoverDismissTimer();
        m_calendarWindow->Show(true, PopupOpenSource::Click);
    }
}

void Application::ToggleCalendar() {
    if (!m_calendarWindow) return;

    ULONGLONG now = GetTickCount64();
    // Debounce duplicate click notifications (e.g. WM_LBUTTONUP + NIN_SELECT from one physical click)
    if (now - m_lastClickTime < 250) {
        return;
    }
    m_lastClickTime = now;

    if (m_calendarWindow->IsVisible()) {
        CancelHoverDismissTimer();
        m_calendarWindow->Show(false);
    } else {
        // Debounce: if the popup was just dismissed via focus loss (e.g. user clicked
        // the tray icon while the popup was open), ignore this toggle so it stays closed.
        if (now - m_calendarWindow->GetLastDismissTime() < 250) {
            return;
        }
        CancelHoverDismissTimer();
        m_calendarWindow->Show(true, PopupOpenSource::Click);
    }
}

void Application::InitializeSettings() {
    HKEY hKey = nullptr;
    DWORD disposition = 0;
    LSTATUS status = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\CalendarFlyout",
        0, nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_QUERY_VALUE | KEY_SET_VALUE,
        nullptr, &hKey, &disposition
    );

    if (status == ERROR_SUCCESS && hKey != nullptr) {
        DWORD firstRun = 0;
        DWORD size = sizeof(DWORD);
        DWORD type = REG_DWORD;
        LSTATUS queryStatus = RegQueryValueExW(hKey, L"FirstRun", nullptr, &type, reinterpret_cast<LPBYTE>(&firstRun), &size);

        if (queryStatus != ERROR_SUCCESS || firstRun == 0) {
            // First launch defaults: ShowOnHover OFF, RunAtStartup ON
            m_showOnHover = false;
            DWORD val = 0;
            RegSetValueExW(hKey, L"ShowOnHover", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&val), sizeof(DWORD));
            firstRun = 1;
            RegSetValueExW(hKey, L"FirstRun", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&firstRun), sizeof(DWORD));

            SetRunAtStartupEnabled(true);
        } else {
            DWORD showHoverVal = 1;
            size = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"ShowOnHover", nullptr, &type, reinterpret_cast<LPBYTE>(&showHoverVal), &size) == ERROR_SUCCESS) {
                m_showOnHover = (showHoverVal != 0);
            } else {
                m_showOnHover = false;
            }
        }

        // One-time migration: builds before settings version 2 defaulted ShowOnHover ON.
        // Switch that old default OFF once, while preserving any later user changes.
        DWORD settingsVersion = 0;
        size = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"SettingsVersion", nullptr, &type,
                             reinterpret_cast<LPBYTE>(&settingsVersion), &size) != ERROR_SUCCESS || settingsVersion < 2) {
            m_showOnHover = false;
            DWORD hoverDisabled = 0;
            RegSetValueExW(hKey, L"ShowOnHover", 0, REG_DWORD,
                           reinterpret_cast<const BYTE*>(&hoverDisabled), sizeof(DWORD));
            settingsVersion = 2;
            RegSetValueExW(hKey, L"SettingsVersion", 0, REG_DWORD,
                           reinterpret_cast<const BYTE*>(&settingsVersion), sizeof(DWORD));
        }

        RegCloseKey(hKey);
    } else {
        m_showOnHover = false;
    }
}

void Application::SetShowOnHoverEnabled(bool enabled) {
    m_showOnHover = enabled;
    if (!m_showOnHover) {
        CancelTrayHoverTimer();
        CancelHoverDismissTimer();
    }
    HKEY hKey = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\CalendarFlyout", 0, nullptr,
                        REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        DWORD val = enabled ? 1 : 0;
        RegSetValueExW(hKey, L"ShowOnHover", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&val), sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

bool Application::IsRunAtStartupEnabled() const {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        wchar_t pathBuf[MAX_PATH];
        DWORD size = sizeof(pathBuf);
        DWORD type = REG_SZ;
        LSTATUS status = RegQueryValueExW(hKey, L"CalendarFlyout", nullptr, &type, reinterpret_cast<LPBYTE>(pathBuf), &size);
        RegCloseKey(hKey);
        return (status == ERROR_SUCCESS);
    }
    return false;
}

void Application::SetRunAtStartupEnabled(bool enabled) {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        if (enabled) {
            wchar_t exePath[MAX_PATH];
            DWORD len = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            if (len > 0 && len < MAX_PATH) {
                std::wstring formatted = L"\"" + std::wstring(exePath) + L"\"";
                RegSetValueExW(
                    hKey,
                    L"CalendarFlyout",
                    0,
                    REG_SZ,
                    reinterpret_cast<const BYTE*>(formatted.c_str()),
                    static_cast<DWORD>((formatted.length() + 1) * sizeof(wchar_t))
                );
            }
        } else {
            RegDeleteValueW(hKey, L"CalendarFlyout");
        }
        RegCloseKey(hKey);
    }
}

} // namespace CalendarFlyout
