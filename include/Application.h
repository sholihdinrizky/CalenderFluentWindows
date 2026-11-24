#pragma once

#include <windows.h>
#include <memory>
#include "TrayIcon.h"

namespace CalendarFlyout {

class Application {
public:
    Application(HINSTANCE hInstance);
    ~Application();

    bool Initialize();
    int Run();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void OnTrayNotification(WPARAM wParam, LPARAM lParam);
    void OnActivateInstance();
    void ToggleCalendar();

    HINSTANCE m_hInstance{nullptr};
    HWND m_hwndController{nullptr};
    std::unique_ptr<TrayIcon> m_trayIcon;
    UINT m_taskbarCreatedMsg{0};
    UINT m_activateInstanceMsg{0};
};

} // namespace CalendarFlyout
