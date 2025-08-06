@echo off
echo Setting up SDL2 for CMake...

REM SDL2 경로 직접 지정
set SDL2_DIR=C:\msys64\ucrt64
set SDL2_INCLUDE_DIR=C:\msys64\ucrt64\include\SDL2
set SDL2_LIBRARY=C:\msys64\ucrt64\lib\libSDL2.dll.a

echo SDL2 paths:
echo   Include: %SDL2_INCLUDE_DIR%
echo   Library: %SDL2_LIBRARY%
echo.

REM CMake 재구성
echo Reconfiguring with SDL2...
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
    -DSDL2_INCLUDE_DIR="%SDL2_INCLUDE_DIR%" ^
    -DSDL2_LIBRARY="%SDL2_LIBRARY%" ^
    -DSDL2_FOUND=TRUE

if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo Configuration successful!
echo Now run build_test.bat to build the project.
pause