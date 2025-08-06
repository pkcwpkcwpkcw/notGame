@echo off
echo ===================================
echo   Installing vcpkg
echo ===================================
echo.

cd /d C:\

echo Cloning vcpkg repository...
git clone https://github.com/Microsoft/vcpkg.git

if %ERRORLEVEL% NEQ 0 (
    echo Failed to clone vcpkg!
    echo Please check your internet connection and git installation.
    pause
    exit /b 1
)

cd vcpkg

echo.
echo Running bootstrap...
call bootstrap-vcpkg.bat

if %ERRORLEVEL% NEQ 0 (
    echo Bootstrap failed!
    pause
    exit /b 1
)

echo.
echo Integrating with Visual Studio...
vcpkg integrate install

if %ERRORLEVEL% NEQ 0 (
    echo Integration failed!
    pause
    exit /b 1
)

echo.
echo ===================================
echo vcpkg installation complete!
echo ===================================
echo.

echo Installing SDL2 and other dependencies...
vcpkg install sdl2:x64-windows opengl:x64-windows glad:x64-windows imgui[sdl2-binding,opengl3-binding]:x64-windows glm:x64-windows nlohmann-json:x64-windows

echo.
echo All dependencies installed!
echo.
echo You can now go back to the project directory and build.
echo.

cd /d C:\project\notgame3

pause