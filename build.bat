@echo off



if not defined DevEnvDir (
    call "D:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
)



IF NOT EXIST build mkdir build


xcopy /s /y /q lib\d3dcompiler_47.dll build


pushd build
cl -Zi -FC ..\app.cpp user32.lib Gdi32.lib Winmm.lib
popd