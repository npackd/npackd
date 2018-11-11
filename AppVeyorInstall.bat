echo on

rem This script is used by AppVeyor automatic builds to install the necessary
rem software dependencies.

msiexec.exe /qn /i https://github.com/tim-lebedkov/npackd-cpp/releases/download/version_1.24.8/NpackdCL32-1.24.8.msi

SET NPACKD_CL=C:\Program Files (x86)\NpackdCL
"%npackd_cl%\ncl" set-repo -u https://npackd.appspot.com/rep/recent-xml -u https://npackd.appspot.com/rep/xml?tag=stable -u https://npackd.appspot.com/rep/xml?tag=stable64 -u https://npackd.appspot.com/rep/recent-xml?tag=untested -u https://npackd.appspot.com/rep/xml?tag=libs -u https://npackd.appspot.com/rep/xml?tag=unstable
if %errorlevel% neq 0 exit /b %errorlevel%

"%npackd_cl%\ncl" detect
if %errorlevel% neq 0 exit /b %errorlevel%

"%npackd_cl%\ncl" set-install-dir -f "C:\Program Files (x86)"
if %errorlevel% neq 0 exit /b %errorlevel%

"%npackd_cl%\ncl" add -p com.googlecode.windows-package-manager.NpackdCL
if %errorlevel% neq 0 exit /b %errorlevel%

"%npackd_cl%\ncl" add -p com.googlecode.windows-package-manager.NpackdInstallerHelper
if %errorlevel% neq 0 exit /b %errorlevel%

"%npackd_cl%\ncl" add -p nircmd64 -v 2.75
if %errorlevel% neq 0 exit /b %errorlevel%

"%npackd_cl%\ncl" add -p com.googlecode.windows-package-manager.CLU -v 1.0.1
if %errorlevel% neq 0 exit /b %errorlevel%

"%npackd_cl%\ncl" add -p sysinternals-suite
if %errorlevel% neq 0 exit /b %errorlevel%

path

where git
