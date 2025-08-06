@echo off
echo Checking available imgui features...
echo.

C:\vcpkg\vcpkg.exe search imgui --x-full-desc

echo.
echo ===================================
echo To see all features for imgui:
C:\vcpkg\vcpkg.exe depend-info imgui

pause