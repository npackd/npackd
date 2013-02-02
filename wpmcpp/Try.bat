call Paths.bat

mkdir .Build
mkdir .Build\Release-64

set oldpath=%path%
set path=%mingw_path%/bin

cd .Build\Release-64
"%qt_path%/bin/qmake.exe" ../../wpmcpp.pro -r -spec win32-g++ "DEFINES+=QUAZIP_STATIC=1" CONFIG+=release "INCLUDEPATH+=%quazip_path%/quazip %zlib_path% %msi_path%" "QMAKE_LIBDIR+=%quazip_path%/quazip/release %zlib_path% %msi_path%"
if %errorlevel% neq 0 goto err

"%mingw_path%\bin\mingw32-make.exe"
if %errorlevel% neq 0 goto err

set path=%oldpath%
cd ..\..
.Build\Release-64\release\wpmcpp.exe
goto :eof

:err
set path=%oldpath%
cd ..\..
rundll32 user32.dll,MessageBeep -1
