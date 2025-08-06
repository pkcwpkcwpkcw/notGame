@echo off
echo ===================================
echo   NOT Gate Sandbox Build Test
echo ===================================
echo.

REM Visual Studio 2022 빌드
echo Configuring with CMake...
cmake -B build -G "Visual Studio 17 2022" -A x64

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: CMake configuration failed!
    echo Please check if SDL2 is installed correctly.
    pause
    exit /b 1
)

echo.
echo Building project...
cmake --build build --config Debug

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo ===================================
echo Build successful!
echo.
echo Executable location:
echo   build\bin\Debug\notgame.exe
echo.
echo Run the program? (Y/N)
set /p RUNPROG=

if /i "%RUNPROG%"=="Y" (
    echo Running NOT Gate Sandbox...
    build\bin\Debug\notgame.exe
)

pause