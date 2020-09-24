@echo off
if not exist ..\build\win_x64_vulkan (
    mkdir ..\build\win_x64_vulkan
)
cd ..\build\win_x64_vulkan
cmake ..\..\ -G "Visual Studio 16" -A x64 -DCMAKE_INSTALL_PREFIX="sdk" -DALIMER_GRAPHICS_API="Vulkan"
cd ..\..\
pause