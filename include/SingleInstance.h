#pragma once

#include <windows.h>

namespace CalendarFlyout {

class SingleInstance {
public:
    SingleInstance();
    ~SingleInstance();

    // Non-copyable
    SingleInstance(const SingleInstance&) = delete;
    SingleInstance& operator=(const SingleInstance&) = delete;

    // Returns true if this is the only running instance.
    // If another instance already exists, notifies it and returns false.
    bool IsUniqueInstance();

private:
    HANDLE m_mutexHandle{nullptr};
    bool m_isUnique{false};
};

} // namespace CalendarFlyout
