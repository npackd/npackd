call Paths.bat
set PATH=%quazip_path%\quazip\release;%zlib_path%;%mingw_path%\bin;%qt_path%\bin
"%gdb_path%\bin\gdb.exe" .Build\Release-64\release\wpmcpp.exe
