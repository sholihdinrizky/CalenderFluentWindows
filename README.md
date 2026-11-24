# CalendarFlyout

An ultra-lightweight, native Windows 11 calendar flyout utility written in Modern C++20 using pure Win32 and GDI/DWM APIs.

## Highlights
- **Zero Heavy Frameworks:** No Electron, no WebView2, no WinUI 3, no XAML, no .NET. Pure Win32 C++20.
- **Resource Footprint:**
  - Executable size: **~172 KB** (budget: < 5 MB).
  - Idle Private Bytes: **~2.3 MB** (budget: < 8 MB, ideal: 2–5 MB).
  - Idle CPU: **0.0%** (fully event-driven message loop, no polling).
  - Background processes: exactly 1 process.
- **Strictly Native Dependencies:** Dynamically links only to standard Windows 10/11 system DLLs (`KERNEL32`, `USER32`, `GDI32`, `SHELL32`, `ucrtbase`).

## Building

### Requirements
- **Compiler:** MSYS2 UCRT64 GCC 16.2.0 (or MSVC x64)
- **Build System:** CMake $\ge$ 3.20 and Ninja (or MinGW Makefiles)

### Build Command
```cmd
build.bat
```
or with CMake directly:
```cmd
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
Output binary: `build/CalendarFlyout.exe`
