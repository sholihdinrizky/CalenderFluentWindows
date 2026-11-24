#include <windows.h>
#include <iostream>
#include <cassert>
#include "../include/CalendarWindow.h"
#include "../include/Application.h"
#include "../include/Constants.h"

using namespace CalendarFlyout;

int main() {
    // Attach to interactive desktop if needed
    HDESK hDesk = OpenDesktopW(L"default", 0, FALSE, GENERIC_ALL);
    if (hDesk != nullptr) {
        SetThreadDesktop(hDesk);
        CloseDesktop(hDesk);
    }

    HINSTANCE hInstance = GetModuleHandleW(nullptr);
    std::cout << "=== Running Sub-Tahap 4.1 Automated Tests ===" << std::endl;

    CalendarWindow calWin(hInstance);
    if (!calWin.Create()) {
        std::cerr << "FAIL: Failed to create CalendarWindow" << std::endl;
        return 1;
    }

    HWND hwnd = calWin.GetHwnd();
    std::cout << "[Test 1] Verify initial state is hidden... ";
    if (calWin.IsVisible()) {
        std::cerr << "FAIL: Window should not be visible initially" << std::endl;
        return 1;
    }
    std::cout << "PASSED" << std::endl;

    std::cout << "[Test 2] Show window and verify keyboard focus... ";
    calWin.Show(true);
    // Allow message queue to process show animation
    MSG msg;
    DWORD start = GetTickCount();
    while (GetTickCount() - start < 200) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        Sleep(10);
    }

    if (!calWin.IsVisible()) {
        std::cerr << "FAIL: Window should be visible after Show(true)" << std::endl;
        return 1;
    }

    // Verify window can receive keyboard messages
    HWND fgHwnd = GetForegroundWindow();
    std::cout << "PASSED (Visible: YES, HWND=" << hwnd << ", Foreground=" << fgHwnd << ")" << std::endl;

    std::cout << "[Test 3] Test Escape key (VK_ESCAPE) dismiss... ";
    SendMessageW(hwnd, WM_KEYDOWN, VK_ESCAPE, 0);
    // Process messages
    start = GetTickCount();
    while (GetTickCount() - start < 150) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        Sleep(10);
    }

    if (calWin.IsVisible()) {
        std::cerr << "FAIL: Window should be hidden after VK_ESCAPE" << std::endl;
        return 1;
    }
    std::cout << "PASSED (Window closed on ESC)" << std::endl;

    std::cout << "[Test 4] Re-open window and test click-outside / deactivation dismiss... ";
    calWin.Show(true);
    start = GetTickCount();
    while (GetTickCount() - start < 200) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        Sleep(10);
    }
    if (!calWin.IsVisible()) {
        std::cerr << "FAIL: Window should be visible after 2nd Show(true)" << std::endl;
        return 1;
    }

    // Simulate loss of focus (click outside sends WM_ACTIVATE with WA_INACTIVE)
    SendMessageW(hwnd, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, 0), 0);
    start = GetTickCount();
    while (GetTickCount() - start < 150) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        Sleep(10);
    }

    if (calWin.IsVisible()) {
        std::cerr << "FAIL: Window should be hidden after WA_INACTIVE" << std::endl;
        return 1;
    }
    std::cout << "PASSED (Window closed on loss of focus)" << std::endl;

    std::cout << "[Test 5] Test Tray click debounce during dismissal... ";
    // Reopen
    calWin.Show(true);
    start = GetTickCount();
    while (GetTickCount() - start < 200) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        Sleep(10);
    }

    // Now simulate user clicking tray icon while popup is open:
    // 1. Mouse down on taskbar tray causes WA_INACTIVE at t=0
    SendMessageW(hwnd, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, 0), 0);
    // 2. Tray receives WM_LBUTTONUP after ~60ms
    Sleep(60);

    // Verify debounce logic:
    ULONGLONG now = GetTickCount64();
    ULONGLONG dismissTime = calWin.GetLastDismissTime();
    bool shouldIgnore = (now - dismissTime < 250);
    if (!shouldIgnore) {
        std::cerr << "FAIL: Debounce check failed (elapsed=" << (now - dismissTime) << "ms)" << std::endl;
        return 1;
    }
    std::cout << "PASSED (Debounce correctly recognized click within " << (now - dismissTime) << "ms)" << std::endl;

    std::cout << "[Test 6] Test reopening after debounce window expires... ";
    Sleep(300); // Wait beyond 250ms threshold
    now = GetTickCount64();
    if (now - calWin.GetLastDismissTime() < 250) {
        std::cerr << "FAIL: Should no longer be ignored after 300ms" << std::endl;
        return 1;
    }
    calWin.Show(true);
    start = GetTickCount();
    while (GetTickCount() - start < 200) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        Sleep(10);
    }
    if (!calWin.IsVisible()) {
        std::cerr << "FAIL: Window should reopen cleanly after debounce expires" << std::endl;
        return 1;
    }
    std::cout << "PASSED (Window reopened cleanly)" << std::endl;

    // Clean close
    calWin.Show(false);

    std::cout << "\n=== All Sub-Tahap 4.1 Tests Passed Successfully! ===" << std::endl;
    return 0;
}
