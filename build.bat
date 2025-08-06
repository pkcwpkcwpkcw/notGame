@echo off
echo Building NotGame...
cmake --build build --config Release
if %ERRORLEVEL% EQU 0 (
    echo Build successful!
) else (
    echo Build failed!
    pause
)