set mingw_path=C:\QtSDK-1.2.1\mingw
set qt_path=C:\QtSDK-1.2.1\Desktop\Qt\4.8.1\mingw
set ai_path=C:\Program Files (x86)\Advanced_Installer_Freeware
set quazip_path=C:\ProgramFiles\QuaZIP
set zlib_path=C:\ProgramFiles\zlib
set sevenzipa_path=C:\Program Files (x86)\7-ZIP_A
set msi_path=C:\ProgramFiles\msi

rem compiling
set path=%mingw_path%\bin
cd ..\wpmcpp-build-desktop
"%qt_path%\bin\qmake.exe" ..\wpmcpp\wpmcpp.pro -r -spec win32-g++ CONFIG+=release "INCLUDEPATH+=%quazip_path%\quazip %zlib_path% %msi_path%" "QMAKE_LIBDIR+=%quazip_path%\quazip\release %zlib_path% %msi_path%"
if %errorlevel% neq 0 exit /b %errorlevel%

"%mingw_path%\bin\mingw32-make.exe"
if %errorlevel% neq 0 exit /b %errorlevel%

cd ..\wpmcpp

mkdir .Build
mkdir .Build\32

rem copying files
copy "%mingw_path%\bin\libgcc_s_dw2-1.dll" .Build\32\
if %errorlevel% neq 0 exit /b %errorlevel%

copy "%mingw_path%\bin\mingwm10.dll" .Build\32\
if %errorlevel% neq 0 exit /b %errorlevel%

copy LICENSE.txt .Build\32\
if %errorlevel% neq 0 exit /b %errorlevel%

copy CrystalIcons_LICENSE.txt .Build\32\
if %errorlevel% neq 0 exit /b %errorlevel%

copy "..\wpmcpp-build-desktop\release\wpmcpp.exe" .Build\32\npackdg.exe
if %errorlevel% neq 0 exit /b %errorlevel%

copy "%qt_path%\bin\QtCore4.dll" .Build\32\
if %errorlevel% neq 0 exit /b %errorlevel%

copy "%qt_path%\bin\QtGui4.dll" .Build\32\
if %errorlevel% neq 0 exit /b %errorlevel%

copy "%qt_path%\bin\QtNetwork4.dll" .Build\32\
if %errorlevel% neq 0 exit /b %errorlevel%

copy "%qt_path%\bin\QtXml4.dll" .Build\32\
if %errorlevel% neq 0 exit /b %errorlevel%

copy "%quazip_path%\quazip\release\quazip.dll" .Build\32\
if %errorlevel% neq 0 exit /b %errorlevel%

copy "%zlib_path%\zlib1.dll" .Build\32\
if %errorlevel% neq 0 exit /b %errorlevel%

rem creating .zip
cd .Build\32\
"%sevenzipa_path%\7za" a ..\Npackd.zip *
if %errorlevel% neq 0 exit /b %errorlevel%
cd ..\..

rem creating .msi
"%ai_path%\bin\x86\AdvancedInstaller.com" /build wpmcpp.aip
if %errorlevel% neq 0 exit /b %errorlevel%
