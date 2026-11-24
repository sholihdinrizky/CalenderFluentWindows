#include <iostream>
#include <cassert>
#include "../include/DateService.h"

using namespace CalendarFlyout;

int main() {
    std::cout << "=== Running DateService Unit Tests ===" << std::endl;

    // 1. Leap Year Verification
    std::cout << "[Test 1] Leap year tests..." << std::endl;
    assert(DateService::IsLeapYear(2024) == true);
    assert(DateService::IsLeapYear(2028) == true);
    assert(DateService::IsLeapYear(2000) == true);
    assert(DateService::IsLeapYear(2026) == false);
    assert(DateService::IsLeapYear(2025) == false);
    assert(DateService::IsLeapYear(1900) == false);
    assert(DateService::IsLeapYear(2100) == false);
    std::cout << "  -> Passed!" << std::endl;

    // 2. Days in month verification
    std::cout << "[Test 2] Days in month tests..." << std::endl;
    assert(DateService::GetDaysInMonth(2024, 2) == 29); // Leap Feb
    assert(DateService::GetDaysInMonth(2026, 2) == 28); // Non-leap Feb
    assert(DateService::GetDaysInMonth(2000, 2) == 29);
    assert(DateService::GetDaysInMonth(1900, 2) == 28);
    assert(DateService::GetDaysInMonth(2026, 1) == 31);
    assert(DateService::GetDaysInMonth(2026, 4) == 30);
    assert(DateService::GetDaysInMonth(2026, 12) == 31);
    std::cout << "  -> Passed!" << std::endl;

    // 3. First day of week verification (0 = Sunday, 1 = Monday, ..., 6 = Saturday)
    std::cout << "[Test 3] First day of week tests..." << std::endl;
    // Nov 1, 2026 is Sunday (0)
    assert(DateService::GetFirstDayOfWeek(2026, 11) == 0);
    // Feb 1, 2024 was Thursday (4)
    assert(DateService::GetFirstDayOfWeek(2024, 2) == 4);
    // Jan 1, 2026 was Thursday (4)
    assert(DateService::GetFirstDayOfWeek(2026, 1) == 4);
    std::cout << "  -> Passed!" << std::endl;

    // 4. 42-cell grid verification
    std::cout << "[Test 4] 42-cell calendar grid tests for Nov 2026..." << std::endl;
    auto gridNov2026 = DateService::GenerateCalendarGrid(2026, 11, 2026, 11, 24);
    assert(gridNov2026.size() == 42);

    // Nov 1 is Sunday, so 0 leading days from October. First cell should be Nov 1!
    assert(gridNov2026[0].day == 1);
    assert(gridNov2026[0].month == 11);
    assert(gridNov2026[0].isCurrentMonth == true);
    assert(gridNov2026[0].isSunday == true);

    // Cell index 23 is Nov 24, today!
    assert(gridNov2026[23].day == 24);
    assert(gridNov2026[23].isToday == true);

    // Nov 30 is cell index 29 (Monday)
    assert(gridNov2026[29].day == 30);
    assert(gridNov2026[29].isCurrentMonth == true);

    // Cell index 30 should be Dec 1 (trailing next month, muted)
    assert(gridNov2026[30].day == 1);
    assert(gridNov2026[30].month == 12);
    assert(gridNov2026[30].isCurrentMonth == false);

    // Check Sunday alignment across all 42 cells
    for (size_t i = 0; i < 42; ++i) {
        if (i % 7 == 0) {
            assert(gridNov2026[i].isSunday == true);
        } else {
            assert(gridNov2026[i].isSunday == false);
        }
    }
    std::cout << "  -> Passed!" << std::endl;

    // 5. Grid for February 2024 (Leap year, starts on Thursday)
    std::cout << "[Test 5] 42-cell grid for Feb 2024 (Leap year)..." << std::endl;
    auto gridFeb2024 = DateService::GenerateCalendarGrid(2024, 2, 2024, 2, 29);
    assert(gridFeb2024.size() == 42);
    // Feb 1 was Thursday (column index 4). Leading cells: Sun Jan 28, Mon Jan 29, Tue Jan 30, Wed Jan 31.
    assert(gridFeb2024[0].day == 28 && gridFeb2024[0].month == 1 && !gridFeb2024[0].isCurrentMonth);
    assert(gridFeb2024[1].day == 29 && gridFeb2024[1].month == 1);
    assert(gridFeb2024[2].day == 30 && gridFeb2024[2].month == 1);
    assert(gridFeb2024[3].day == 31 && gridFeb2024[3].month == 1);
    assert(gridFeb2024[4].day == 1 && gridFeb2024[4].month == 2 && gridFeb2024[4].isCurrentMonth);
    // Feb 29 (leap day)
    assert(gridFeb2024[4 + 28].day == 29 && gridFeb2024[4 + 28].month == 2 && gridFeb2024[4 + 28].isToday);
    // Trailing cells: Mar 1, Mar 2, ...
    assert(gridFeb2024[33].day == 1 && gridFeb2024[33].month == 3 && !gridFeb2024[33].isCurrentMonth);
    std::cout << "  -> Passed!" << std::endl;

    std::cout << "\nALL DATE LOGIC AND GRID TESTS PASSED 100%!" << std::endl;
    return 0;
}
