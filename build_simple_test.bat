@echo off
echo Building simple SDL2 test...

REM Simple compile command
cl /EHsc /MD /I"C:\msys64\ucrt64\include" /I"C:\msys64\ucrt64\include\SDL2" ^
   src\test_simple.cpp ^
   /link /LIBPATH:"C:\msys64\ucrt64\lib" SDL2.lib SDL2main.lib ^
   /SUBSYSTEM:CONSOLE /OUT:test_simple.exe

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build successful!
echo.
echo Running test...
test_simple.exe

pause