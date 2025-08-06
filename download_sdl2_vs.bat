@echo off
echo ===================================
echo   Download SDL2 for Visual Studio
echo ===================================
echo.

echo SDL2 for Visual Studio needs to be downloaded from:
echo https://www.libsdl.org/download-2.0.php
echo.
echo Download: SDL2-devel-2.X.X-VC.zip (Visual C++ 32/64-bit)
echo.
echo After downloading:
echo 1. Extract the ZIP file
echo 2. Copy the SDL2-2.X.X folder to C:\SDL2
echo 3. The structure should be:
echo    C:\SDL2\
echo    ├── include\
echo    │   └── (SDL.h and other headers)
echo    ├── lib\
echo    │   ├── x64\
echo    │   │   ├── SDL2.dll
echo    │   │   ├── SDL2.lib
echo    │   │   └── SDL2main.lib
echo    │   └── x86\
echo    └── README-SDL.txt
echo.
echo Press any key to open the download page...
pause >nul

start https://github.com/libsdl-org/SDL/releases/latest

echo.
echo After extracting SDL2 to C:\SDL2, press any key to continue...
pause

echo.
echo Configuring project with Visual Studio SDL2...
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
    -DUSE_SDL2=ON ^
    -DSDL2_INCLUDE_DIR="C:/SDL2/include" ^
    -DSDL2_LIBRARY="C:/SDL2/lib/x64/SDL2.lib" ^
    -DSDL2MAIN_LIBRARY="C:/SDL2/lib/x64/SDL2main.lib"

if %ERRORLEVEL% NEQ 0 (
    echo Configuration failed!
    pause
    exit /b 1
)

echo.
echo Copying SDL2.dll to output directory...
copy "C:\SDL2\lib\x64\SDL2.dll" "build\bin\Debug\" >nul 2>&1

echo.
echo Configuration complete! Now run build_test.bat
pause