#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace CalendarFlyout {

struct CalendarDay {
    int year{0};
    int month{0};
    int day{0};
    bool isCurrentMonth{false};
    bool isToday{false};
    bool isSunday{false};
};

class DateService {
public:
    DateService();

    // Updates to latest system time
    void RefreshCurrentDate();

    // Current real-world date
    int GetCurrentYear() const { return m_currentYear; }
    int GetCurrentMonth() const { return m_currentMonth; }
    int GetCurrentDay() const { return m_currentDay; }

    // Viewed date (for browsing)
    int GetViewYear() const { return m_viewYear; }
    int GetViewMonth() const { return m_viewMonth; }
    void SetViewDate(int year, int month);
    void NavigatePreviousMonth();
    void NavigateNextMonth();
    void ResetToToday();

    // Leap year calculation using std::chrono
    static bool IsLeapYear(int year);

    // Days in a given month (1..12)
    static int GetDaysInMonth(int year, int month);

    // 0 = Sunday, 1 = Monday, ..., 6 = Saturday
    static int GetFirstDayOfWeek(int year, int month);

    // Generates the 42 cells (7 columns x 6 rows) for the current view
    std::vector<CalendarDay> GenerateCalendarGrid() const;
    static std::vector<CalendarDay> GenerateCalendarGrid(int viewYear, int viewMonth, int todayYear, int todayMonth, int todayDay);

    // Formatted strings
    std::wstring GetFormattedViewHeader() const; // e.g. "November 2026"
    std::wstring GetFormattedFullDate() const;   // e.g. "Tuesday, November 24, 2026" (always today)

private:
    int m_currentYear{2026};
    int m_currentMonth{11};
    int m_currentDay{24};

    int m_viewYear{2026};
    int m_viewMonth{11};
};

} // namespace CalendarFlyout
