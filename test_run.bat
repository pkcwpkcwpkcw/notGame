@echo off
echo === Testing Gate Placement System ===
cd build\bin\Debug
notgame.exe 2>&1 | findstr /C:"===" /C:"Gate" /C:"Placement" /C:"FRAME" /C:"initialized"
echo.
echo === Test Complete ===
pause