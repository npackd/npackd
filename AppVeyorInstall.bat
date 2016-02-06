echo on

rem This script is used by AppVeyor automatic builds to install the necessary
rem software dependencies.

msiexec.exe /qn /i https://github.com/tim-lebedkov/packages/releases/download/initial/NpackdCL-1.20.5.msi
SET NPACKD_CL=C:\Program Files (x86)\NpackdCL
"%npackd_cl%\ncl" add-repo --url=https://npackd.appspot.com/rep/recent-xml
if %errorlevel% neq 0 exit /b %errorlevel%
"%npackd_cl%\ncl" add-repo --url=http://npackd.appspot.com/rep/recent-xml?tag=untested
if %errorlevel% neq 0 exit /b %errorlevel%
"%npackd_cl%\ncl" add-repo --url=https://npackd.appspot.com/rep/xml?tag=libs
if %errorlevel% neq 0 exit /b %errorlevel%
"%npackd_cl%\ncl" add-repo --url=https://npackd.appspot.com/rep/xml?tag=unstable
if %errorlevel% neq 0 exit /b %errorlevel%

"%npackd_cl%\ncl" detect
if %errorlevel% neq 0 exit /b %errorlevel%
"%npackd_cl%\ncl" set-install-dir -f "C:\Program Files (x86)"
if %errorlevel% neq 0 exit /b %errorlevel%
"%npackd_cl%\ncl" add -p nircmd64 -v 2.75
if %errorlevel% neq 0 exit /b %errorlevel%
"%npackd_cl%\ncl" add -p com.googlecode.windows-package-manager.CLU -v 1.0.1
if %errorlevel% neq 0 exit /b %errorlevel%
"%npackd_cl%\ncl" add -p sysinternals-suite
rem if %errorlevel% neq 0 exit /b %errorlevel%

path

