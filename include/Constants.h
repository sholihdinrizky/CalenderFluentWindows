#pragma once

#include <windows.h>

namespace CalendarFlyout::Constants {
    // Registered custom window message for tray icon callbacks
    inline constexpr UINT WM_TRAY_CALLBACK = WM_USER + 1;

    // Window class name for controller
    inline constexpr const wchar_t* CONTROLLER_CLASS_NAME = L"CalendarFlyout_ControllerWindow";

    // Named mutex for single-instance protection
    inline constexpr const wchar_t* SINGLE_INSTANCE_MUTEX = L"Local\\CalendarFlyout_SingleInstance_Mutex_9B87F1";

    // Registered window message for activating existing instance
    inline constexpr const wchar_t* ACTIVATE_MSG_NAME = L"CalendarFlyout_ActivateInstance_9B87F1";
}
