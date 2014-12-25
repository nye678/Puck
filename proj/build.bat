@echo off
cls

call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64

pushd ..\build

del *.pdb

cl /Z7 /Fm ^
/I"..\..\jrmem\inc" ^
/I"..\..\jrmath\inc" ^
/I"..\..\jrrender\inc" ^
..\code\puck_game_ub.cpp ^
/LD /link /incremental:no /out:puck.dll /pdb:puck%random%.pdb

cl /Z7 /Fepuck_win32 ^
/I"C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include" ^
/I"..\..\jrmem\inc" ^
/I"..\..\jrmath\inc" ^
/I"..\..\jrrender\inc" ^
..\code\puck_win32_ub.cpp ^
/link /incremental:no

popd