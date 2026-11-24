#pragma once

#include <windows.h>
#include <memory>
#include "DateService.h"
#include "ThemeManager.h"
#include "CalendarRenderer.h"

namespace CalendarFlyout {

class CalendarWindow {
public:
    CalendarWindow(HINSTANCE hInstance);
    ~CalendarWindow();

    bool Create();
    void Show(bool show);
    bool IsVisible() const;
    HWND GetHwnd() const { return m_hwnd; }

    void PositionNearTray(POINT trayPt = {0, 0});
    void Refresh();

    DateService& GetDateService() { return m_dateService; }
    ThemeManager& GetThemeManager() { return m_themeManager; }
    CalendarRenderer& GetRenderer() { return m_renderer; }

    // Testing and preview helpers for Phase 3
    void SetHoverCellForTest(int cellIndex);
    void CaptureToFile(const std::wstring& outPngOrBmpPath);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void OnPaint(HWND hwnd);
    void UpdateDwmAttributes();
    void RecalculateSizeAndDpi();

    HINSTANCE m_hInstance{nullptr};
    HWND m_hwnd{nullptr};
    DateService m_dateService;
    ThemeManager m_themeManager;
    CalendarRenderer m_renderer;

    int m_hoveredCellIndex{-1};
    bool m_hoveredPrev{false};
    bool m_hoveredNext{false};
    int m_currentDpi{96};
};

} // namespace CalendarFlyout
