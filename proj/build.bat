@echo off
cls
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64

pushd ..\..\jrcore\proj
call .\build.bat x64, dbg
popd

pushd ..\build
del *.pdb
copy /Y ..\..\jrcore\build\x64\debug\jrcore_ub.lib .\jrcore.lib
copy /Y ..\..\jrcore\build\x64\debug\jrcore.dll .\jrcore.dll
copy /Y ..\..\jrcore\build\x64\debug\jrcore.pdb .\jrcore.pdb

cl /Z7 /Fm ^
/I"..\..\jrcore\inc" ^
..\code\puck_game_ub.cpp jrcore.lib ^
/LD /link /incremental:no /out:puck.dll /pdb:puck%random%.pdb

cl /Z7 /Fepuck_win32 ^
/I"C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include" ^
/I"..\..\jrcore\inc" ^
..\code\puck_win32_ub.cpp jrcore.lib ^
/link /incremental:no

popd