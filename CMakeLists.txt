cmake_minimum_required(VERSION 3.20)
project(CalendarFlyout LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Unicode and lean Windows headers
add_compile_definitions(UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8 /O2)
else()
    add_compile_options(-Wall -Wextra -O2)
    # Target GUI subsystem (no console window) and unicode entry point
    add_link_options(-mwindows -municode)
    # Static link GCC runtime (libgcc, libstdc++, winpthread) to eliminate all external runtime DLL dependencies
    add_link_options(-static -static-libgcc -static-libstdc++ -s)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/Application.cpp
    src/TrayIcon.cpp
    src/SingleInstance.cpp
    src/DateService.cpp
    src/ThemeManager.cpp
    src/CalendarRenderer.cpp
    src/CalendarWindow.cpp
)

set(HEADERS
    include/Application.h
    include/TrayIcon.h
    include/SingleInstance.h
    include/Constants.h
    include/Resource.h
    include/DateService.h
    include/ThemeManager.h
    include/CalendarRenderer.h
    include/CalendarWindow.h
)

# Resources
set(RESOURCE_FILES
    resources/resource.rc
)

add_executable(CalendarFlyout WIN32
    ${SOURCES}
    ${HEADERS}
    ${RESOURCE_FILES}
)

target_include_directories(CalendarFlyout PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(CalendarFlyout PRIVATE
    user32
    gdi32
    shell32
    dwmapi
    uxtheme
)
