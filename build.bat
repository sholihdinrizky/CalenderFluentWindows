@echo off
setlocal

set "PATH=C:\Users\sholihuddin rizky\AppData\Local\Programs\CMake\bin;C:\msys64\ucrt64\bin;%PATH%"

echo === Configuring CalendarFlyout with CMake ===
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe -DCMAKE_RC_COMPILER=C:/msys64/ucrt64/bin/windres.exe
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    exit /b %ERRORLEVEL%
)

echo === Building CalendarFlyout Release ===
cmake --build build --config Release
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    exit /b %ERRORLEVEL%
)

echo === Build complete! Output located at build\CalendarFlyout.exe ===
