#include "CalendarRenderer.h"
#include <vector>
#include <algorithm>

namespace CalendarFlyout {

namespace {
    inline int Scale(int val, int dpi) {
        return MulDiv(val, dpi, 96);
    }

    HFONT CreateDpiFont(const wchar_t* faceName, int ptSize, int weight, int dpi) {
        int height = -MulDiv(ptSize, dpi, 72);
        return CreateFontW(
            height,
            0,
            0,
            0,
            weight,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            faceName
        );
    }
}

CalendarRenderer::CalendarRenderer() {
    CalculateLayout(m_metrics.width, m_metrics.height);
}

CalendarRenderer::~CalendarRenderer() {
    CleanupFonts();
}

void CalendarRenderer::CleanupFonts() {
    if (m_hFontHeader) { DeleteObject(m_hFontHeader); m_hFontHeader = nullptr; }
    if (m_hFontWeekday) { DeleteObject(m_hFontWeekday); m_hFontWeekday = nullptr; }
    if (m_hFontDate) { DeleteObject(m_hFontDate); m_hFontDate = nullptr; }
    if (m_hFontBottomDate) { DeleteObject(m_hFontBottomDate); m_hFontBottomDate = nullptr; }
    m_lastFontDpi = 0;
}

void CalendarRenderer::EnsureFonts() {
    if (m_lastFontDpi == m_metrics.dpi && m_hFontHeader != nullptr) {
        return;
    }
    CleanupFonts();

    // Prefer Segoe UI Variable Display for Windows 11; fallback to Segoe UI
    m_hFontHeader = CreateDpiFont(L"Segoe UI Variable Display", 15, FW_SEMIBOLD, m_metrics.dpi);
    m_hFontWeekday = CreateDpiFont(L"Segoe UI", 9, FW_NORMAL, m_metrics.dpi);
    m_hFontDate = CreateDpiFont(L"Segoe UI", 11, FW_NORMAL, m_metrics.dpi);
    m_hFontBottomDate = CreateDpiFont(L"Segoe UI Variable Display", 13, FW_SEMIBOLD, m_metrics.dpi);

    m_lastFontDpi = m_metrics.dpi;
}

void CalendarRenderer::UpdateDpi(int dpi, int windowWidth, int windowHeight) {
    m_metrics.dpi = dpi;
    CalculateLayout(windowWidth, windowHeight);
}

void CalendarRenderer::CalculateLayout(int width, int height) {
    m_metrics.width = width;
    m_metrics.height = height;

    int dpi = m_metrics.dpi;
    m_metrics.paddingH = Scale(18, dpi);
    m_metrics.paddingV = Scale(16, dpi);

    // 1. Header (Month/Year text on left, Chevrons on right)
    int headerHeight = Scale(32, dpi);
    m_metrics.headerRect = {
        m_metrics.paddingH,
        m_metrics.paddingV,
        width - m_metrics.paddingH,
        m_metrics.paddingV + headerHeight
    };

    int btnSize = Scale(28, dpi);
    int btnY = m_metrics.headerRect.top + (headerHeight - btnSize) / 2;

    m_metrics.nextButtonRect = {
        width - m_metrics.paddingH - btnSize,
        btnY,
        width - m_metrics.paddingH,
        btnY + btnSize
    };

    m_metrics.prevButtonRect = {
        m_metrics.nextButtonRect.left - btnSize - Scale(4, dpi),
        btnY,
        m_metrics.nextButtonRect.left - Scale(4, dpi),
        btnY + btnSize
    };

    // 2. Weekdays
    int headerBottomSpacing = Scale(8, dpi);
    int weekdayHeight = Scale(24, dpi);
    int weekdaysTop = m_metrics.headerRect.bottom + headerBottomSpacing;

    m_metrics.weekdaysRect = {
        m_metrics.paddingH,
        weekdaysTop,
        width - m_metrics.paddingH,
        weekdaysTop + weekdayHeight
    };

    // 3. 42-cell Grid (7 columns x 6 rows)
    int gridTopSpacing = Scale(4, dpi);
    int gridTop = m_metrics.weekdaysRect.bottom + gridTopSpacing;
    int gridWidth = width - 2 * m_metrics.paddingH;

    m_metrics.cellWidth = gridWidth / 7;
    // Base cell height
    m_metrics.cellHeight = Scale(32, dpi);
    int totalGridHeight = m_metrics.cellHeight * 6;

    m_metrics.gridRect = {
        m_metrics.paddingH,
        gridTop,
        m_metrics.paddingH + m_metrics.cellWidth * 7,
        gridTop + totalGridHeight
    };

    // Circle diameter for hover / today selection
    m_metrics.circleDiameter = std::min(m_metrics.cellWidth - Scale(4, dpi), m_metrics.cellHeight - Scale(2, dpi));
    if (m_metrics.circleDiameter < Scale(24, dpi)) m_metrics.circleDiameter = Scale(24, dpi);

    // 4. Bottom Full-Date Display
    int bottomSpacing = Scale(6, dpi);   // was 12 — reduced to remove excess whitespace
    int bottomTop = m_metrics.gridRect.bottom + bottomSpacing;
    int bottomHeight = Scale(26, dpi);   // was 32 — slightly tighter

    m_metrics.bottomDateRect = {
        m_metrics.paddingH,
        bottomTop,
        width - m_metrics.paddingH,
        bottomTop + bottomHeight
    };

    // Store the computed minimum window height so CalendarWindow can query it
    m_metrics.computedHeight = bottomTop + bottomHeight + Scale(10, dpi); // 10px bottom padding
}

int CalendarRenderer::HitTestCell(POINT pt) const {
    if (!PtInRect(&m_metrics.gridRect, pt)) {
        return -1;
    }

    int relX = pt.x - m_metrics.gridRect.left;
    int relY = pt.y - m_metrics.gridRect.top;

    int col = relX / m_metrics.cellWidth;
    int row = relY / m_metrics.cellHeight;

    if (col >= 0 && col < 7 && row >= 0 && row < 6) {
        int index = row * 7 + col;
        if (index >= 0 && index < 42) {
            return index;
        }
    }
    return -1;
}

bool CalendarRenderer::HitTestPrev(POINT pt) const {
    return PtInRect(&m_metrics.prevButtonRect, pt) != FALSE;
}

bool CalendarRenderer::HitTestNext(POINT pt) const {
    return PtInRect(&m_metrics.nextButtonRect, pt) != FALSE;
}

void CalendarRenderer::DrawFilledCircle(HDC hdc, int cx, int cy, int diameter, COLORREF color) {
    // GDI supersampling antialiasing:
    // Draw the circle at 3x size into an off-screen bitmap, then StretchBlt it back
    // down with HALFTONE mode. Windows blends neighboring pixels during downscale,
    // producing smooth edges — entirely pure GDI, no extra DLLs.
    const int SCALE = 3;
    int bigDiam = diameter * SCALE;

    HDC bigDC = CreateCompatibleDC(hdc);
    HBITMAP bigBmp = CreateCompatibleBitmap(hdc, bigDiam, bigDiam);
    HGDIOBJ oldBigBmp = SelectObject(bigDC, bigBmp);

    // Fill the oversized bitmap with the same background color we'll be blending into.
    // We use black here as a transparent sentinel; the final blit uses SRCCOPY after
    // compositing, so we fill with the destination area first by copying it in.
    // Simpler approach: use the window background color. Since we don't have it here,
    // we BitBlt the destination region into bigDC at 1:1 (stretched 3x) so the background
    // is correct before drawing the ellipse.
    // Actually — simpler and correct: fill bigDC with the destination pixels (StretchBlt from hdc).
    int r = diameter / 2;
    StretchBlt(bigDC, 0, 0, bigDiam, bigDiam, hdc, cx - r, cy - r, diameter, diameter, SRCCOPY);

    // Draw the circle at 3x size
    HBRUSH hBrush = CreateSolidBrush(color);
    HGDIOBJ oldBrush = SelectObject(bigDC, hBrush);
    HGDIOBJ oldPen = SelectObject(bigDC, GetStockObject(NULL_PEN));
    Ellipse(bigDC, 0, 0, bigDiam, bigDiam);
    SelectObject(bigDC, oldPen);
    SelectObject(bigDC, oldBrush);
    DeleteObject(hBrush);

    // Downscale back to destination with HALFTONE for smooth blending
    SetStretchBltMode(hdc, HALFTONE);
    SetBrushOrgEx(hdc, 0, 0, nullptr);
    StretchBlt(hdc, cx - r, cy - r, diameter, diameter, bigDC, 0, 0, bigDiam, bigDiam, SRCCOPY);
    SetStretchBltMode(hdc, COLORONCOLOR); // restore default

    SelectObject(bigDC, oldBigBmp);
    DeleteObject(bigBmp);
    DeleteDC(bigDC);
}

void CalendarRenderer::DrawChevron(HDC hdc, const RECT& rc, bool isLeft, COLORREF color, int dpi) {
    int cx = rc.left + (rc.right - rc.left) / 2;
    int cy = rc.top + (rc.bottom - rc.top) / 2;
    int arm = Scale(4, dpi);
    int span = Scale(5, dpi);

    int penWidth = std::max(1, Scale(2, dpi));
    HPEN hPen = CreatePen(PS_SOLID, penWidth, color);
    HGDIOBJ oldPen = SelectObject(hdc, hPen);

    POINT pts[3];
    if (isLeft) {
        pts[0] = {cx + arm, cy - span};
        pts[1] = {cx - arm, cy};
        pts[2] = {cx + arm, cy + span};
    } else {
        pts[0] = {cx - arm, cy - span};
        pts[1] = {cx + arm, cy};
        pts[2] = {cx - arm, cy + span};
    }

    Polyline(hdc, pts, 3);

    SelectObject(hdc, oldPen);
    DeleteObject(hPen);
}

void CalendarRenderer::Render(
    HDC hdc,
    const RECT& clientRect,
    const DateService& dateService,
    const ThemeColors& theme,
    int hoveredCellIndex,
    bool hoveredPrev,
    bool hoveredNext
) {
    EnsureFonts();

    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    // 1. Off-screen Double-Buffered Memory DC
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    HGDIOBJ oldBitmap = SelectObject(memDC, memBitmap);

    SetBkMode(memDC, TRANSPARENT);

    // 2. Window Background
    HBRUSH hBrBg = CreateSolidBrush(theme.background);
    FillRect(memDC, &clientRect, hBrBg);
    DeleteObject(hBrBg);

    // 3. No explicit border rectangle — DWM DWMWCP_ROUND handles window outline
    //    Drawing a Rectangle() here would paint sharp corners over DWM's rounded ones.

    // 4. Header: Month and Year (e.g., "November 2026")
    std::wstring headerText = dateService.GetFormattedViewHeader();
    SelectObject(memDC, m_hFontHeader);
    SetTextColor(memDC, theme.textNormal);

    RECT headerTextRect = m_metrics.headerRect;
    headerTextRect.right = m_metrics.prevButtonRect.left - Scale(8, m_metrics.dpi);
    DrawTextW(
        memDC,
        headerText.c_str(),
        -1,
        &headerTextRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX
    );

    // 5. Navigation Chevrons
    // Previous Month Button
    if (hoveredPrev) {
        int cx = (m_metrics.prevButtonRect.left + m_metrics.prevButtonRect.right) / 2;
        int cy = (m_metrics.prevButtonRect.top + m_metrics.prevButtonRect.bottom) / 2;
        DrawFilledCircle(memDC, cx, cy, m_metrics.prevButtonRect.right - m_metrics.prevButtonRect.left, theme.chevronHoverBg);
    }
    DrawChevron(memDC, m_metrics.prevButtonRect, true, theme.chevronColor, m_metrics.dpi);

    // Next Month Button
    if (hoveredNext) {
        int cx = (m_metrics.nextButtonRect.left + m_metrics.nextButtonRect.right) / 2;
        int cy = (m_metrics.nextButtonRect.top + m_metrics.nextButtonRect.bottom) / 2;
        DrawFilledCircle(memDC, cx, cy, m_metrics.nextButtonRect.right - m_metrics.nextButtonRect.left, theme.chevronHoverBg);
    }
    DrawChevron(memDC, m_metrics.nextButtonRect, false, theme.chevronColor, m_metrics.dpi);

    // 6. Weekday Header (Su, Mo, Tu, We, Th, Fr, Sa)
    SelectObject(memDC, m_hFontWeekday);
    SetTextColor(memDC, theme.textWeekday);

    static const wchar_t* const weekdayLabels[7] = {
        L"Su", L"Mo", L"Tu", L"We", L"Th", L"Fr", L"Sa"
    };

    for (int col = 0; col < 7; ++col) {
        RECT rcCol{
            m_metrics.gridRect.left + col * m_metrics.cellWidth,
            m_metrics.weekdaysRect.top,
            m_metrics.gridRect.left + (col + 1) * m_metrics.cellWidth,
            m_metrics.weekdaysRect.bottom
        };
        DrawTextW(
            memDC,
            weekdayLabels[col],
            -1,
            &rcCol,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX
        );
    }

    // 7. 42-cell Monthly Grid
    std::vector<CalendarDay> grid = dateService.GenerateCalendarGrid();
    SelectObject(memDC, m_hFontDate);

    for (int i = 0; i < 42 && i < static_cast<int>(grid.size()); ++i) {
        const auto& cell = grid[i];
        int col = i % 7;
        int row = i / 7;

        RECT cellRect{
            m_metrics.gridRect.left + col * m_metrics.cellWidth,
            m_metrics.gridRect.top + row * m_metrics.cellHeight,
            m_metrics.gridRect.left + (col + 1) * m_metrics.cellWidth,
            m_metrics.gridRect.top + (row + 1) * m_metrics.cellHeight
        };

        int cx = (cellRect.left + cellRect.right) / 2;
        int cy = (cellRect.top + cellRect.bottom) / 2;
        bool isHovered = (i == hoveredCellIndex);

        COLORREF textColor = theme.textNormal;

        if (cell.isToday) {
            // Today's date always receives circular accent highlight.
            // On hover: draw a slightly lighter outer ring to show interactivity.
            if (isHovered) {
                // Draw a slightly larger, semi-transparent ring around the accent circle
                DrawFilledCircle(memDC, cx, cy, m_metrics.circleDiameter + Scale(4, m_metrics.dpi), theme.normalHoverBg);
            }
            DrawFilledCircle(memDC, cx, cy, m_metrics.circleDiameter, theme.accentColor);
            textColor = theme.accentTextColor;
        } else if (cell.isSunday) {
            if (isHovered) {
                // Sunday on hover: red circular background, white text
                DrawFilledCircle(memDC, cx, cy, m_metrics.circleDiameter, theme.sundayHoverBg);
                textColor = theme.sundayHoverText;
            } else {
                // Normal Sunday: subtle red color
                textColor = cell.isCurrentMonth ? theme.sundayRed : theme.textMuted;
            }
        } else {
            if (isHovered) {
                // Normal date on hover: subtle gray circular background
                DrawFilledCircle(memDC, cx, cy, m_metrics.circleDiameter, theme.normalHoverBg);
                textColor = theme.textNormal;
            } else {
                textColor = cell.isCurrentMonth ? theme.textNormal : theme.textMuted;
            }
        }

        std::wstring dayStr = std::to_wstring(cell.day);
        SetTextColor(memDC, textColor);
        DrawTextW(
            memDC,
            dayStr.c_str(),
            -1,
            &cellRect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX
        );
    }

    // 8. Bottom Full-Date Display (ALWAYS today's real date)
    SelectObject(memDC, m_hFontBottomDate);
    SetTextColor(memDC, theme.textNormal);

    std::wstring fullDateStr = dateService.GetFormattedFullDate();
    DrawTextW(
        memDC,
        fullDateStr.c_str(),
        -1,
        &m_metrics.bottomDateRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX
    );

    // 9. Blit Double-Buffered Image to Screen
    BitBlt(hdc, clientRect.left, clientRect.top, width, height, memDC, 0, 0, SRCCOPY);

    // 10. Clean up Memory DC and Bitmap
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

} // namespace CalendarFlyout
