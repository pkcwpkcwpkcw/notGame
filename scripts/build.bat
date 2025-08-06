@echo off
echo Building NOT Gate Game...
echo.

cd build

:: PATH가 설정되어 있지 않은 경우를 위한 백업
if not exist "%ProgramFiles%\CMake\bin\cmake.exe" goto :CheckMSBuild
set PATH=%ProgramFiles%\CMake\bin;%PATH%

:CheckMSBuild
if not exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" goto :Build
set PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin;%PATH%

:Build
:: CMake 재구성이 필요한지 확인
if not exist "NotGame.sln" (
    echo Solution file not found. Running CMake...
    cmake .. -G "Visual Studio 17 2022" -A x64
    if ERRORLEVEL 1 (
        echo CMake configuration failed!
        pause
        exit /b 1
    )
)

:: 빌드 실행
echo Building project...
msbuild NotGame.sln /p:Configuration=Debug /p:Platform=x64 /v:m

if ERRORLEVEL 1 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build successful!
echo Executable: build\bin\Debug\notgame.exe
echo.
pause