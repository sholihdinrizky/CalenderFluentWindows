#include <windows.h>
#include "Application.h"
#include "SingleInstance.h"

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE /*hPrevInstance*/,
    PWSTR /*lpCmdLine*/,
    int /*nCmdShow*/
) {
    // 0. Ensure attachment to the user's interactive desktop if running in an isolated sub-desktop
    HDESK hDesk = OpenDesktopW(L"default", 0, FALSE, GENERIC_ALL);
    if (hDesk != nullptr) {
        SetThreadDesktop(hDesk);
        CloseDesktop(hDesk);
    }

    // 1. Single-instance protection
    CalendarFlyout::SingleInstance singleInstance;
    if (!singleInstance.IsUniqueInstance()) {
        // Another instance is already running; it has been notified to activate.
        // Clean exit with 0.
        return 0;
    }

    // 2. Initialize application controller
    CalendarFlyout::Application app(hInstance);
    if (!app.Initialize()) {
        return 1;
    }

    // 3. Run message loop
    return app.Run();
}
