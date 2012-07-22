set mingw_path=C:\Program Files\MinGW-w64_64_bit
set qt_path=C:\Users\t\projects\qt-mingw-64-builder\qt
set quazip_path=C:\ProgramFiles\QuaZIP-x86_64-w64
set zlib_path=C:\ProgramFiles\zlib-x86_64-w64

set PATH=%quazip_path%\quazip\release;%zlib_path%;%mingw_path%\bin;%qt_path%\bin
"C:\Program Files\GDB_64_bit\bin\gdb.exe" ..\wpmcpp-build-desktop-64\debug\wpmcpp.exe
