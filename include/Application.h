#pragma once

#include <windows.h>
#include <memory>
#include "TrayIcon.h"
#include "CalendarWindow.h"

namespace CalendarFlyout {

class Application {
public:
    Application(HINSTANCE hInstance);
    ~Application();

    bool Initialize();
    int Run();

    CalendarWindow* GetCalendarWindow() { return m_calendarWindow.get(); }
    void ToggleCalendar();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void OnTrayNotification(WPARAM wParam, LPARAM lParam);
    void OnActivateInstance();
    void StartTrayHoverTimer();
    void CancelTrayHoverTimer();

    HINSTANCE m_hInstance{nullptr};
    HWND m_hwndController{nullptr};
    std::unique_ptr<TrayIcon> m_trayIcon;
    std::unique_ptr<CalendarWindow> m_calendarWindow;

    UINT m_taskbarCreatedMsg{0};
    UINT m_activateInstanceMsg{0};
    bool m_trayHoverTimerActive{false};
};

} // namespace CalendarFlyout
