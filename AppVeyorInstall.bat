echo on

rem This script is used by AppVeyor automatic builds to install the necessary
rem software dependencies.

msiexec.exe /qn /i https://github.com/tim-lebedkov/npackd-cpp/releases/download/version_1.25/NpackdCL64-1.25.0.msi

SET NPACKD_CL=C:\Program Files\NpackdCL
"%npackd_cl%\ncl" set-repo -u https://npackd.appspot.com/rep/recent-xml -u https://npackd.appspot.com/rep/xml?tag=stable -u https://npackd.appspot.com/rep/xml?tag=stable64 -u https://npackd.appspot.com/rep/recent-xml?tag=untested -u https://npackd.appspot.com/rep/xml?tag=libs -u https://npackd.appspot.com/rep/xml?tag=unstable
if %errorlevel% neq 0 exit /b %errorlevel%

"%npackd_cl%\ncl" help
if %errorlevel% neq 0 exit /b %errorlevel%

"%npackd_cl%\ncl" detect
if %errorlevel% neq 0 exit /b %errorlevel%

"%npackd_cl%\ncl" set-install-dir -f "C:\Program Files (x86)"
if %errorlevel% neq 0 exit /b %errorlevel%

"%npackd_cl%\ncl" add -p com.googlecode.windows-package-manager.NpackdCL -p com.googlecode.windows-package-manager.NpackdInstallerHelper -p com.googlecode.windows-package-manager.CLU -v 1.0.1 -p sysinternals-suite -p se.haxx.curl.CURL64
if %errorlevel% neq 0 exit /b %errorlevel%

go get github.com/kbinani/screenshot
if %errorlevel% neq 0 exit /b %errorlevel%
