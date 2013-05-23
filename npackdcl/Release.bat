verify

set version=1.18

set onecmd="%npackd_cl%\npackdcl.exe" path --package=com.nokia.QtDev-i686-w64-Npackd-Release --versions=[4.8.2,4.8.2]
for /f "usebackq delims=" %%x in (`%%onecmd%%`) do set qt=%%x
if %errorlevel% neq 0 exit /b %errorlevel%

set onecmd="%npackd_cl%\npackdcl.exe" path --package=net.sourceforge.mingw-w64.MinGWW64 --versions=[4.7.2,4.7.2]
for /f "usebackq delims=" %%x in (`%%onecmd%%`) do set mingw=%%x
if %errorlevel% neq 0 exit /b %errorlevel%

set onecmd="%npackd_cl%\npackdcl.exe" path --package=com.advancedinstaller.AdvancedInstallerFreeware --versions=[10,20]
for /f "usebackq delims=" %%x in (`%%onecmd%%`) do set ai=%%x
if %errorlevel% neq 0 exit /b %errorlevel%

set onecmd="%npackd_cl%\npackdcl.exe" path --package=org.7-zip.SevenZIPA --versions=[9.20,9.20]
for /f "usebackq delims=" %%x in (`%%onecmd%%`) do set sevenzipa=%%x
if %errorlevel% neq 0 exit /b %errorlevel%

set output=..\npackdcl-release

rmdir /s /q %output%
rmdir /s /q ..\npackdcl-i686

set path=%mingw%\bin
"%qt%\bin\qmake" clean
if %errorlevel% neq 0 exit /b %errorlevel%

"%qt%\bin\qmake"
if %errorlevel% neq 0 exit /b %errorlevel%

mkdir %output%\files
if %errorlevel% neq 0 exit /b %errorlevel%

copy LICENSE.txt %output%\files\
if %errorlevel% neq 0 goto out

copy CrystalIcons_LICENSE.txt %output%\files\
if %errorlevel% neq 0 goto out

copy "..\npackdcl-i686\release\npackdcl.exe" %output%\files\
if %errorlevel% neq 0 goto out

cd %output%\files
"%sevenzipa%\7za" a ..\NpackdCL-%version%.zip *
if %errorlevel% neq 0 goto out
cd ..\..\npackdcl

rem creating .msi
"%ai%\bin\x86\AdvancedInstaller.com" /build NpackdCL.aip
if %errorlevel% neq 0 goto out


