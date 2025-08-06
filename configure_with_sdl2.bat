@echo off
echo ===================================
echo   Configuring with SDL2 enabled
echo ===================================
echo.

REM Clean build directory
echo Cleaning build directory...
rmdir /S /Q build 2>nul
mkdir build

REM Configure with SDL2 enabled
echo Configuring CMake with SDL2...
cmake -B build -G "Visual Studio 17 2022" -A x64 -DUSE_SDL2=ON

if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo Configuration successful!
echo.
echo Building project...
cmake --build build --config Debug

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ===================================
echo Build successful!
echo.
echo Run the program? (Y/N)
set /p RUNPROG=

if /i "%RUNPROG%"=="Y" (
    echo Running NOT Gate Sandbox...
    build\bin\Debug\notgame.exe
)

pause