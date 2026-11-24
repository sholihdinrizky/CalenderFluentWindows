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

    // Initialize and show tray icon
    m_trayIcon = std::make_unique<TrayIcon>(m_hwndController, m_hInstance);
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
        }
        return 0;
    }

    if (msg == m_activateInstanceMsg && m_activateInstanceMsg != 0) {
        OnActivateInstance();
        return 0;
    }

    switch (msg) {
    case Constants::WM_TRAY_CALLBACK:
        OnTrayNotification(wParam, lParam);
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
    case WM_LBUTTONUP:
        ToggleCalendar();
        break;

    case WM_RBUTTONUP:
    case WM_CONTEXTMENU: {
        POINT pt;
        GetCursorPos(&pt);
        if (m_trayIcon) {
            m_trayIcon->ShowContextMenu(pt);
        }
        break;
    }
    }
}

void Application::OnActivateInstance() {
    // When a second instance is launched, activate/show the calendar popup
    if (m_calendarWindow && !m_calendarWindow->IsVisible()) {
        m_calendarWindow->Show(true);
    }
}

void Application::ToggleCalendar() {
    if (m_calendarWindow) {
        if (m_calendarWindow->IsVisible()) {
            m_calendarWindow->Show(false);
        } else {
            // Debounce: if the popup was just dismissed via focus loss (e.g. user clicked
            // the tray icon while the popup was open), ignore this toggle so it stays closed.
            ULONGLONG now = GetTickCount64();
            if (now - m_calendarWindow->GetLastDismissTime() < 250) {
                return;
            }
            m_calendarWindow->Show(true);
        }
    }
}

} // namespace CalendarFlyout
