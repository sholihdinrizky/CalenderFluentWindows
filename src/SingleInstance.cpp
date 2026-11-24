#include "SingleInstance.h"
#include "Constants.h"

namespace CalendarFlyout {

SingleInstance::SingleInstance() {
    m_mutexHandle = CreateMutexW(nullptr, TRUE, Constants::SINGLE_INSTANCE_MUTEX);
    if (m_mutexHandle != nullptr) {
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            m_isUnique = false;
        } else {
            m_isUnique = true;
        }
    } else {
        // Fallback: If mutex creation failed for some system reason, allow start
        m_isUnique = true;
    }
}

SingleInstance::~SingleInstance() {
    if (m_mutexHandle != nullptr) {
        if (m_isUnique) {
            ReleaseMutex(m_mutexHandle);
        }
        CloseHandle(m_mutexHandle);
        m_mutexHandle = nullptr;
    }
}

bool SingleInstance::IsUniqueInstance() {
    if (!m_isUnique) {
        // Notify the existing instance to activate/show itself
        UINT activateMsg = RegisterWindowMessageW(Constants::ACTIVATE_MSG_NAME);
        HWND existingWnd = FindWindowW(Constants::CONTROLLER_CLASS_NAME, nullptr);
        if (existingWnd != nullptr && activateMsg != 0) {
            PostMessageW(existingWnd, activateMsg, 0, 0);
        }
    }
    return m_isUnique;
}

} // namespace CalendarFlyout
