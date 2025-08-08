@echo off
cd build\bin\Release
echo Running notgame.exe with selection debugging...
echo.
echo Instructions:
echo - Press N to enter gate placement mode
echo - Click to place gates
echo - Press ESC to exit placement mode
echo - Click on a gate to select it (should turn yellow)
echo - Check console for debug messages
echo.
notgame.exe
pause