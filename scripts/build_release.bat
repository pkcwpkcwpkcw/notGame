@echo off
REM Build script for Windows release with bundled SDL2

echo ====================================
echo   NotGame Release Build Script
echo ====================================
echo.

REM Clean previous build
if exist build_release (
    echo Cleaning previous build...
    rmdir /s /q build_release
)

REM Create build directory
mkdir build_release
cd build_release

REM Configure with bundled SDL2
echo Configuring CMake with bundled SDL2...
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DUSE_SDL2=ON ^
    -DUSE_BUNDLED_SDL2=ON ^
    -DSDL2_STATIC_LINK=OFF ^
    -DBUILD_TESTS=OFF

if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

REM Build
echo.
echo Building Release...
cmake --build . --config Release --parallel

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b 1
)

REM Create distribution folder
echo.
echo Creating distribution package...
if not exist dist mkdir dist
if not exist dist\NotGame mkdir dist\NotGame

REM Copy executable
copy bin\Release\notgame.exe dist\NotGame\

REM Copy SDL2.dll (if dynamically linked)
if exist bin\Release\SDL2.dll (
    copy bin\Release\SDL2.dll dist\NotGame\
)

REM Copy assets
if exist ..\assets (
    xcopy /E /I /Y ..\assets dist\NotGame\assets
)

REM Copy licenses
if not exist dist\NotGame\licenses mkdir dist\NotGame\licenses
copy ..\LICENSE dist\NotGame\licenses\LICENSE.txt

REM Create README for distribution
echo NotGame - NOT Gate Logic Sandbox > dist\NotGame\README.txt
echo. >> dist\NotGame\README.txt
echo To run the game, simply execute notgame.exe >> dist\NotGame\README.txt
echo. >> dist\NotGame\README.txt
echo For more information, visit: https://github.com/pkcwpkcwpkcw/notGame >> dist\NotGame\README.txt

echo.
echo ====================================
echo   Build Complete!
echo ====================================
echo.
echo Distribution package created in: build_release\dist\NotGame\
echo.
echo You can now:
echo 1. Test the game: dist\NotGame\notgame.exe
echo 2. Create ZIP: Right-click dist\NotGame and "Send to > Compressed folder"
echo.

cd ..
pause