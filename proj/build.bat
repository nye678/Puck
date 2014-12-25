@echo off
cls
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64

pushd ..\build

cl /Zi /Fm ^
/I"..\..\jrmem\inc" ^
/I"..\..\jrmath\inc" ^
/I"..\..\jrrender\inc" ^
..\code\puck_game_ub.cpp ^
/LD /link /incremental:no /out:puck.dll

cl /Zi /Fepuck ^
/I"C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include" ^
/I"..\..\jrmem\inc" ^
/I"..\..\jrmath\inc" ^
/I"..\..\jrrender\inc" ^
..\code\puck_win32_ub.cpp ^
/link /incremental:no
popd