@echo off
REM ══════════════════════════════════════════════════════════════
REM  Multithreaded Sensor System - Windows Build Script
REM ══════════════════════════════════════════════════════════════
REM
REM  Kullanım:
REM    build.bat              → Projeyi derle ve çalıştır
REM    build.bat test         → Testleri çalıştır
REM    build.bat clean        → Build klasörünü temizle
REM    build.bat all          → Temizle + Derle + Test + Çalıştır
REM
REM ══════════════════════════════════════════════════════════════

REM Visual Studio Developer ortamını yükle
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64 >nul 2>&1

if "%1"=="clean" goto :clean
if "%1"=="test" goto :test
if "%1"=="all" goto :all

:build
echo.
echo [BUILD] Configuring with CMake...
cmake -S . -B build -G "Ninja" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake configuration failed!
    cmake -S . -B build -G "Ninja"
    exit /b 1
)

echo [BUILD] Compiling...
cmake --build build
if errorlevel 1 (
    echo [ERROR] Build failed!
    exit /b 1
)

echo.
echo [BUILD] Build successful!
echo [BUILD] Running sensor_system...
echo.
build\sensor_system.exe
goto :eof

:test
echo.
echo [TEST] Running tests...
ctest --test-dir build --output-on-failure
goto :eof

:clean
echo.
echo [CLEAN] Removing build directory...
if exist build rmdir /s /q build
echo [CLEAN] Done.
goto :eof

:all
call :clean
call :build
echo.
call :test
goto :eof
