#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include "../include/DateService.h"
#include "../include/ThemeManager.h"
#include "../include/CalendarRenderer.h"
#include "../include/CalendarWindow.h"

using namespace CalendarFlyout;

int main() {
    // Attach to interactive desktop if running in sandbox sub-desktop
    HDESK hDesk = OpenDesktopW(L"default", 0, FALSE, GENERIC_ALL);
    if (hDesk != nullptr) {
        SetThreadDesktop(hDesk);
        CloseDesktop(hDesk);
    }

    HINSTANCE hInstance = GetModuleHandleW(nullptr);
    CalendarWindow calWin(hInstance);
    if (!calWin.Create()) {
        std::cerr << "Failed to create CalendarWindow" << std::endl;
        return 1;
    }

    std::cout << "Generating test visual captures..." << std::endl;

    // 1. Dark Mode - Normal
    calWin.GetThemeManager().SetDarkModeOverride(true);
    calWin.SetHoverCellForTest(-1);
    calWin.CaptureToFile(L"build/dark_mode_normal.bmp");
    std::cout << "Captured: dark_mode_normal.bmp" << std::endl;

    // 2. Dark Mode - Sunday Hover (e.g. cell index 7 = Nov 8, Sunday)
    calWin.SetHoverCellForTest(7);
    calWin.CaptureToFile(L"build/dark_mode_sunday_hover.bmp");
    std::cout << "Captured: dark_mode_sunday_hover.bmp" << std::endl;

    // 3. Dark Mode - Normal Date Hover (e.g. cell index 10 = Nov 11, Wednesday)
    calWin.SetHoverCellForTest(10);
    calWin.CaptureToFile(L"build/dark_mode_date_hover.bmp");
    std::cout << "Captured: dark_mode_date_hover.bmp" << std::endl;

    // 4. Light Mode - Normal
    calWin.GetThemeManager().SetDarkModeOverride(false);
    calWin.SetHoverCellForTest(-1);
    calWin.CaptureToFile(L"build/light_mode_normal.bmp");
    std::cout << "Captured: light_mode_normal.bmp" << std::endl;

    // 5. Light Mode - Sunday Hover
    calWin.SetHoverCellForTest(14); // Nov 15, Sunday
    calWin.CaptureToFile(L"build/light_mode_sunday_hover.bmp");
    std::cout << "Captured: light_mode_sunday_hover.bmp" << std::endl;

    std::cout << "All visual captures created successfully!" << std::endl;
    return 0;
}
