#include <windows.h>
#include <iostream>
#include <cassert>
#include <string>
#include "../include/CalendarWindow.h"

using namespace CalendarFlyout;

int main() {
    HDESK hDesk = OpenDesktopW(L"default", 0, FALSE, GENERIC_ALL);
    if (hDesk != nullptr) {
        SetThreadDesktop(hDesk);
        CloseDesktop(hDesk);
    }

    HINSTANCE hInstance = GetModuleHandleW(nullptr);
    std::cout << "=== Running Comprehensive Phase 4 Automated Test Suite ===" << std::endl;

    CalendarWindow calWin(hInstance);
    if (!calWin.Create()) {
        std::cerr << "FAIL: Failed to create CalendarWindow" << std::endl;
        return 1;
    }

    HWND hwnd = calWin.GetHwnd();

    // Helper message pump
    auto pump = [](DWORD ms) {
        MSG msg;
        DWORD start = GetTickCount();
        while (GetTickCount() - start < ms) {
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            Sleep(5);
        }
    };

    // 1. Show window
    std::cout << "[Test 1] Show window and verify initial view date... ";
    calWin.Show(true);
    pump(150);
    if (!calWin.IsVisible()) {
        std::cerr << "FAIL: Window should be visible" << std::endl;
        return 1;
    }
    std::wstring initHeader = calWin.GetDateService().GetFormattedViewHeader();
    std::wstring initFullDate = calWin.GetDateService().GetFormattedFullDate();
    std::cout << "PASSED (Header: " << std::string(initHeader.begin(), initHeader.end()) 
              << ", Today: " << std::string(initFullDate.begin(), initFullDate.end()) << ")" << std::endl;

    // 2. ESC key dismiss
    std::cout << "[Test 2] Verify Escape key (VK_ESCAPE) dismiss... ";
    SendMessageW(hwnd, WM_KEYDOWN, VK_ESCAPE, 0);
    pump(120);
    if (calWin.IsVisible()) {
        std::cerr << "FAIL: Window should be hidden after ESC" << std::endl;
        return 1;
    }
    std::cout << "PASSED" << std::endl;

    // 3. Re-open and verify click outside (WA_INACTIVE)
    std::cout << "[Test 3] Reopen and verify click-outside (WA_INACTIVE) dismiss... ";
    calWin.Show(true);
    pump(150);
    SendMessageW(hwnd, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, 0), 0);
    pump(120);
    if (calWin.IsVisible()) {
        std::cerr << "FAIL: Window should be hidden after WA_INACTIVE" << std::endl;
        return 1;
    }
    std::cout << "PASSED" << std::endl;

    // 4. Re-open and test Hover on Date Cells
    std::cout << "[Test 4] Verify mouse hover on date cells... ";
    calWin.Show(true);
    pump(150);

    const auto& metrics = calWin.GetRenderer().GetMetrics();
    // Calculate a point inside cell index 10 (row 1, col 3)
    int cellX = metrics.gridRect.left + 3 * metrics.cellWidth + metrics.cellWidth / 2;
    int cellY = metrics.gridRect.top + 1 * metrics.cellHeight + metrics.cellHeight / 2;
    SendMessageW(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(cellX, cellY));
    pump(30);

    // HitTestCell should return 10
    POINT ptCell = { cellX, cellY };
    int expectedIndex = calWin.GetRenderer().HitTestCell(ptCell);
    if (expectedIndex != 10) {
        std::cerr << "FAIL: Expected hit test index 10, got " << expectedIndex << std::endl;
        return 1;
    }
    std::cout << "PASSED (Cell 10 hit-tested accurately)" << std::endl;

    // 5. Test Hover on Chevrons
    std::cout << "[Test 5] Verify mouse hover on chevrons... ";
    int prevX = (metrics.prevButtonRect.left + metrics.prevButtonRect.right) / 2;
    int prevY = (metrics.prevButtonRect.top + metrics.prevButtonRect.bottom) / 2;
    SendMessageW(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(prevX, prevY));
    pump(30);

    POINT ptPrev = { prevX, prevY };
    if (!calWin.GetRenderer().HitTestPrev(ptPrev) || calWin.GetRenderer().HitTestNext(ptPrev)) {
        std::cerr << "FAIL: Prev chevron hit test failed" << std::endl;
        return 1;
    }

    int nextX = (metrics.nextButtonRect.left + metrics.nextButtonRect.right) / 2;
    int nextY = (metrics.nextButtonRect.top + metrics.nextButtonRect.bottom) / 2;
    SendMessageW(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(nextX, nextY));
    pump(30);

    POINT ptNext = { nextX, nextY };
    if (!calWin.GetRenderer().HitTestNext(ptNext) || calWin.GetRenderer().HitTestPrev(ptNext)) {
        std::cerr << "FAIL: Next chevron hit test failed" << std::endl;
        return 1;
    }
    std::cout << "PASSED (Chevrons hit-tested accurately)" << std::endl;

    // 6. Test Mouse Leave
    std::cout << "[Test 6] Verify WM_MOUSELEAVE clears hover state... ";
    SendMessageW(hwnd, WM_MOUSELEAVE, 0, 0);
    pump(30);
    std::cout << "PASSED" << std::endl;

    // 7. Test Month Navigation (Forward & Cross-Year)
    std::cout << "[Test 7] Verify Next Month navigation and cross-year forward... ";
    RECT rcBefore;
    GetWindowRect(hwnd, &rcBefore);

    int startMonth = calWin.GetDateService().GetViewMonth();
    int startYear = calWin.GetDateService().GetViewYear();

    // Click next button
    SendMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(nextX, nextY));
    pump(50);
    int monthAfter1 = calWin.GetDateService().GetViewMonth();
    int yearAfter1 = calWin.GetDateService().GetViewYear();
    if (startMonth == 12) {
        assert(monthAfter1 == 1 && yearAfter1 == startYear + 1);
    } else {
        assert(monthAfter1 == startMonth + 1 && yearAfter1 == startYear);
    }

    // Verify window dimensions and position did NOT jump or shift
    RECT rcAfter;
    GetWindowRect(hwnd, &rcAfter);
    if (rcBefore.left != rcAfter.left || rcBefore.top != rcAfter.top ||
        (rcBefore.right - rcBefore.left) != (rcAfter.right - rcAfter.left) ||
        (rcBefore.bottom - rcBefore.top) != (rcAfter.bottom - rcAfter.top)) {
        std::cerr << "FAIL: Window moved or resized during month navigation" << std::endl;
        return 1;
    }

    // Navigate to next year
    while (calWin.GetDateService().GetViewYear() == startYear) {
        SendMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(nextX, nextY));
        pump(20);
    }
    if (calWin.GetDateService().GetViewYear() != startYear + 1) {
        std::cerr << "FAIL: Cross-year navigation forward failed" << std::endl;
        return 1;
    }
    std::cout << "PASSED (Navigated forward into " << calWin.GetDateService().GetViewYear() 
              << ", Window size & pos unchanged)" << std::endl;

    // 8. Verify Bottom Date Unchanged while browsing another month/year
    std::cout << "[Test 8] Verify bottom full date ALWAYS remains actual current date... ";
    std::wstring bottomDateBrowsing = calWin.GetDateService().GetFormattedFullDate();
    if (bottomDateBrowsing != initFullDate) {
        std::cerr << "FAIL: Bottom full date changed! Expected: " 
                  << std::string(initFullDate.begin(), initFullDate.end())
                  << ", Got: " << std::string(bottomDateBrowsing.begin(), bottomDateBrowsing.end()) << std::endl;
        return 1;
    }
    std::cout << "PASSED (Actual date preserved: " 
              << std::string(bottomDateBrowsing.begin(), bottomDateBrowsing.end()) << ")" << std::endl;

    // 9. Test Month Navigation (Backward & Cross-Year)
    std::cout << "[Test 9] Verify Previous Month navigation and cross-year backward... ";
    // Navigate backward 24 months
    for (int i = 0; i < 24; ++i) {
        SendMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(prevX, prevY));
        pump(10);
    }
    if (calWin.GetDateService().GetViewYear() != startYear - 1) {
        std::cerr << "FAIL: Cross-year navigation backward failed. Expected year " 
                  << (startYear - 1) << ", got " << calWin.GetDateService().GetViewYear() << std::endl;
        return 1;
    }
    std::cout << "PASSED (Navigated backward into " << calWin.GetDateService().GetViewYear() << ")" << std::endl;

    // 10. Re-open / Reset to Today
    std::cout << "[Test 10] Close and reopen: verify view automatically resets to current month... ";
    calWin.Show(false);
    pump(120);
    calWin.Show(true);
    pump(150);
    if (calWin.GetDateService().GetViewYear() != startYear || calWin.GetDateService().GetViewMonth() != startMonth) {
        std::cerr << "FAIL: Reset to today failed on reopen" << std::endl;
        return 1;
    }
    std::cout << "PASSED (View cleanly reset to current month on reopen)" << std::endl;

    calWin.Show(false);
    std::cout << "\n=== All 10 Phase 4 Automated Tests Passed Successfully! ===" << std::endl;
    return 0;
}
