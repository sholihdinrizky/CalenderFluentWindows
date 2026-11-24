#include "DateService.h"
#include <windows.h>
#include <array>
#include <sstream>

namespace CalendarFlyout {

DateService::DateService() {
    RefreshCurrentDate();
    m_viewYear = m_currentYear;
    m_viewMonth = m_currentMonth;
}

void DateService::RefreshCurrentDate() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    m_currentYear = st.wYear;
    m_currentMonth = st.wMonth;
    m_currentDay = st.wDay;
}

void DateService::SetViewDate(int year, int month) {
    m_viewYear = year;
    m_viewMonth = month;
    if (m_viewMonth < 1) {
        m_viewMonth = 12;
        m_viewYear--;
    } else if (m_viewMonth > 12) {
        m_viewMonth = 1;
        m_viewYear++;
    }
}

void DateService::NavigatePreviousMonth() {
    m_viewMonth--;
    if (m_viewMonth < 1) {
        m_viewMonth = 12;
        m_viewYear--;
    }
}

void DateService::NavigateNextMonth() {
    m_viewMonth++;
    if (m_viewMonth > 12) {
        m_viewMonth = 1;
        m_viewYear++;
    }
}

void DateService::ResetToToday() {
    RefreshCurrentDate();
    m_viewYear = m_currentYear;
    m_viewMonth = m_currentMonth;
}

bool DateService::IsLeapYear(int year) {
    return std::chrono::year(year).is_leap();
}

int DateService::GetDaysInMonth(int year, int month) {
    if (month < 1 || month > 12) return 30;
    auto ym = std::chrono::year(year) / std::chrono::month(month);
    auto yml = std::chrono::year_month_day_last(ym.year(), std::chrono::month_day_last(ym.month()));
    return static_cast<unsigned int>(yml.day());
}

int DateService::GetFirstDayOfWeek(int year, int month) {
    // Returns 0 for Sunday, 1 for Monday, ..., 6 for Saturday
    auto ymd = std::chrono::year_month_day(
        std::chrono::year(year),
        std::chrono::month(month),
        std::chrono::day(1)
    );
    auto sysDays = std::chrono::sys_days{ymd};
    auto weekday = std::chrono::weekday{sysDays};
    return static_cast<int>(weekday.c_encoding());
}

std::vector<CalendarDay> DateService::GenerateCalendarGrid() const {
    return GenerateCalendarGrid(m_viewYear, m_viewMonth, m_currentYear, m_currentMonth, m_currentDay);
}

std::vector<CalendarDay> DateService::GenerateCalendarGrid(
    int viewYear, int viewMonth, int todayYear, int todayMonth, int todayDay
) {
    std::vector<CalendarDay> grid;
    grid.reserve(42);

    int daysInCurrentMonth = GetDaysInMonth(viewYear, viewMonth);
    int firstDayWeekday = GetFirstDayOfWeek(viewYear, viewMonth);

    // Calculate previous month details
    int prevYear = viewYear;
    int prevMonth = viewMonth - 1;
    if (prevMonth < 1) {
        prevMonth = 12;
        prevYear--;
    }
    int daysInPrevMonth = GetDaysInMonth(prevYear, prevMonth);

    // 1. Leading cells from previous month (muted)
    int leadingDays = firstDayWeekday; // e.g. If Sunday (0), 0 leading cells. If Monday (1), 1 leading cell.
    int startPrevDay = daysInPrevMonth - leadingDays + 1;
    for (int i = 0; i < leadingDays; ++i) {
        int d = startPrevDay + i;
        bool isSun = ((int)grid.size() % 7 == 0);
        bool isToday = (prevYear == todayYear && prevMonth == todayMonth && d == todayDay);
        grid.push_back(CalendarDay{prevYear, prevMonth, d, false, isToday, isSun});
    }

    // 2. Current month cells
    for (int d = 1; d <= daysInCurrentMonth; ++d) {
        bool isSun = ((int)grid.size() % 7 == 0);
        bool isToday = (viewYear == todayYear && viewMonth == todayMonth && d == todayDay);
        grid.push_back(CalendarDay{viewYear, viewMonth, d, true, isToday, isSun});
    }

    // 3. Trailing cells from next month (muted) to complete the 42-cell (6-row x 7-column) grid
    int nextYear = viewYear;
    int nextMonth = viewMonth + 1;
    if (nextMonth > 12) {
        nextMonth = 1;
        nextYear++;
    }

    int nextDay = 1;
    while (grid.size() < 42) {
        bool isSun = ((int)grid.size() % 7 == 0);
        bool isToday = (nextYear == todayYear && nextMonth == todayMonth && nextDay == todayDay);
        grid.push_back(CalendarDay{nextYear, nextMonth, nextDay, false, isToday, isSun});
        nextDay++;
    }

    return grid;
}

std::wstring DateService::GetFormattedViewHeader() const {
    static const wchar_t* const monthNames[] = {
        L"January", L"February", L"March", L"April", L"May", L"June",
        L"July", L"August", L"September", L"October", L"November", L"December"
    };

    if (m_viewMonth >= 1 && m_viewMonth <= 12) {
        std::wstringstream ss;
        ss << monthNames[m_viewMonth - 1] << L" " << m_viewYear;
        return ss.str();
    }
    return L"";
}

std::wstring DateService::GetFormattedFullDate() const {
    // ALWAYS shows real current system date, even when viewing another month
    // Calculate weekday for today
    auto ymd = std::chrono::year_month_day(
        std::chrono::year(m_currentYear),
        std::chrono::month(m_currentMonth),
        std::chrono::day(m_currentDay)
    );
    auto sysDays = std::chrono::sys_days{ymd};
    auto weekday = std::chrono::weekday{sysDays};
    int w = static_cast<int>(weekday.c_encoding());

    static const wchar_t* const weekdayNames[] = {
        L"Sunday", L"Monday", L"Tuesday", L"Wednesday", L"Thursday", L"Friday", L"Saturday"
    };
    static const wchar_t* const monthNames[] = {
        L"January", L"February", L"March", L"April", L"May", L"June",
        L"July", L"August", L"September", L"October", L"November", L"December"
    };

    std::wstringstream ss;
    if (w >= 0 && w <= 6 && m_currentMonth >= 1 && m_currentMonth <= 12) {
        ss << weekdayNames[w] << L", " << monthNames[m_currentMonth - 1] << L" " << m_currentDay << L", " << m_currentYear;
    }
    return ss.str();
}

} // namespace CalendarFlyout
