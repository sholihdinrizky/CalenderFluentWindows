#pragma once

#include <windows.h>
#include <string>
#include "DateService.h"
#include "ThemeManager.h"

namespace CalendarFlyout {

struct LayoutMetrics {
    int dpi{96};
    int width{340};
    int height{324};

    int paddingH{18};
    int paddingV{16};

    RECT headerRect{};
    RECT prevButtonRect{};
    RECT nextButtonRect{};

    RECT weekdaysRect{};
    RECT gridRect{};
    RECT bottomDateRect{};

    int cellWidth{0};
    int cellHeight{0};
    int circleDiameter{0};

    // Computed total content height (populated by CalculateLayout)
    int computedHeight{324};
};

class CalendarRenderer {
public:
    CalendarRenderer();
    ~CalendarRenderer();

    void UpdateDpi(int dpi, int windowWidth, int windowHeight);
    void Render(
        HDC hdc,
        const RECT& clientRect,
        const DateService& dateService,
        const ThemeColors& theme,
        int hoveredCellIndex = -1,
        bool hoveredPrev = false,
        bool hoveredNext = false
    );

    const LayoutMetrics& GetMetrics() const { return m_metrics; }
    int HitTestCell(POINT pt) const;
    bool HitTestPrev(POINT pt) const;
    bool HitTestNext(POINT pt) const;

private:
    void CalculateLayout(int width, int height);
    void EnsureFonts();
    void CleanupFonts();

    static void DrawFilledCircle(HDC hdc, int cx, int cy, int diameter, COLORREF color);
    static void DrawChevron(HDC hdc, const RECT& rc, bool isLeft, COLORREF color, int dpi);

    LayoutMetrics m_metrics;

    HFONT m_hFontHeader{nullptr};
    HFONT m_hFontWeekday{nullptr};
    HFONT m_hFontDate{nullptr};
    HFONT m_hFontBottomDate{nullptr};
    int m_lastFontDpi{0};
};

} // namespace CalendarFlyout
