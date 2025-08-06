@echo off
echo Configuring NotGame with CMake...
cmake -B build -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% EQU 0 (
    echo Configuration successful!
    echo Run 'build.bat' to build the project.
) else (
    echo Configuration failed!
    pause
)