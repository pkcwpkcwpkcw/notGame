@echo off
echo ===================================
echo   Building with vcpkg
echo ===================================
echo.

REM Clean and recreate build directory
echo Cleaning build directory...
rmdir /S /Q build 2>nul
mkdir build

REM Configure with vcpkg toolchain
echo Configuring CMake with vcpkg...
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
    -DUSE_SDL2=ON ^
    -DUSE_OPENGL=ON ^
    -DUSE_IMGUI=ON

if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

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
echo ===================================
echo.

REM Copy SDL2.dll if needed
echo Copying SDL2.dll...
copy "build\vcpkg_installed\x64-windows\bin\SDL2.dll" "build\bin\Debug\" >nul 2>&1

echo Running NOT Gate Sandbox...
build\bin\Debug\notgame.exe

pause