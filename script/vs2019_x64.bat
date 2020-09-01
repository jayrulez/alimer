@echo off
if not exist ..\build\win_x64 (
    mkdir ..\build\win_x64
)
cd ..\build\win_x64
cmake ..\..\ -G "Visual Studio 16" -A x64 -DCMAKE_INSTALL_PREFIX="sdk"
cd ..\..\
pause