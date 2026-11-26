#include "Application.h"
#include "Constants.h"
#include "Resource.h"

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
            ToggleCalendar();
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
        {
            POINT pt;
            GetCursorPos(&pt);
            if (m_trayIcon) {
                m_trayIcon->ShowContextMenu(pt);
            }
        }
        break;

    case NIN_POPUPOPEN:
        // Received when the user hovers the tray icon (NOTIFYICON_VERSION_4).
        // Use a short timer so passing over the icon does not open the popup.
        m_isPointerOverTrayIcon = true;
        CancelHoverDismissTimer();
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
    if (m_calendarWindow) {
        if (m_calendarWindow->IsVisible()) {
            CancelHoverDismissTimer();
            m_calendarWindow->Show(false);
        } else {
            // Debounce: if the popup was just dismissed via focus loss (e.g. user clicked
            // the tray icon while the popup was open), ignore this toggle so it stays closed.
            ULONGLONG now = GetTickCount64();
            if (now - m_calendarWindow->GetLastDismissTime() < 250) {
                return;
            }
            CancelHoverDismissTimer();
            m_calendarWindow->Show(true, PopupOpenSource::Click);
        }
    }
}

} // namespace CalendarFlyout
