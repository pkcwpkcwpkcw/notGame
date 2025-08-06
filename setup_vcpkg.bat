@echo off
echo ===================================
echo   Setting up vcpkg for SDL2
echo ===================================
echo.

REM Check if vcpkg is installed
where vcpkg >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo vcpkg found!
    goto :install_sdl2
)

echo vcpkg not found. Please install vcpkg first:
echo.
echo 1. Clone vcpkg:
echo    git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
echo.
echo 2. Run bootstrap:
echo    C:\vcpkg\bootstrap-vcpkg.bat
echo.
echo 3. Integrate with Visual Studio:
echo    C:\vcpkg\vcpkg integrate install
echo.
echo After installing vcpkg, run this script again.
pause
exit /b 1

:install_sdl2
echo.
echo Installing SDL2 with vcpkg...
vcpkg install sdl2:x64-windows

if %ERRORLEVEL% NEQ 0 (
    echo SDL2 installation failed!
    pause
    exit /b 1
)

echo.
echo SDL2 installed successfully!
echo.
echo Now configuring CMake with vcpkg toolchain...

REM Find vcpkg root
for /f "delims=" %%i in ('where vcpkg') do set VCPKG_PATH=%%~dpi
set VCPKG_ROOT=%VCPKG_PATH:~0,-1%

echo Using vcpkg from: %VCPKG_ROOT%

cmake -B build -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
    -DUSE_SDL2=ON

if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo Configuration successful!
echo Run build_test.bat to build the project.
pause