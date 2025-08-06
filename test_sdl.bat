@echo off
echo ===================================
echo   SDL2 Simple Test
echo ===================================
echo.

echo Building test_simple...
cmake --build build --config Debug --target test_simple

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Copying SDL2.dll...
copy "C:\msys64\ucrt64\bin\SDL2.dll" "build\bin\Debug\" >nul 2>&1

echo Running test_simple.exe...
echo.
build\bin\Debug\test_simple.exe

pause