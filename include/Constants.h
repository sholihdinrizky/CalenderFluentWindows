#pragma once

#include <windows.h>

namespace CalendarFlyout::Constants {
    // Registered custom window message for tray icon callbacks
    inline constexpr UINT WM_TRAY_CALLBACK = WM_USER + 1;

    // Timer used to open the calendar after an intentional tray-icon hover.
    inline constexpr UINT_PTR TIMER_TRAY_HOVER_OPEN = 1;
    inline constexpr UINT TRAY_HOVER_OPEN_DELAY_MS = 400;

    // Timer used to dismiss a hover-opened popup once the pointer leaves both
    // the tray icon and the popup. A short delay avoids flicker during the
    // tray-to-popup transition.
    inline constexpr UINT_PTR TIMER_HOVER_DISMISS = 2;
    inline constexpr UINT HOVER_DISMISS_DELAY_MS = 120;

    // Posted by CalendarWindow when pointer leaves popup client area.
    inline constexpr UINT WM_POPUP_POINTER_LEFT = WM_APP + 2;

    // Window class name for controller
    inline constexpr const wchar_t* CONTROLLER_CLASS_NAME = L"CalendarFlyout_ControllerWindow";

    // Named mutex for single-instance protection
    inline constexpr const wchar_t* SINGLE_INSTANCE_MUTEX = L"Local\\CalendarFlyout_SingleInstance_Mutex_9B87F1";

    // Registered window message for activating existing instance
    inline constexpr const wchar_t* ACTIVATE_MSG_NAME = L"CalendarFlyout_ActivateInstance_9B87F1";
}
