echo on

rem This script is used by AppVeyor automatic builds to install the necessary
rem software dependencies.

msiexec.exe /qn /i http://bit.ly/npackdcl-1_19_13 || exit /b %errorlevel%
SET NPACKD_CL=C:\Program Files (x86)\NpackdCL|| exit /b %errorlevel%
"%npackd_cl%\ncl" add-repo --url=https://npackd.appspot.com/rep/recent-xml || exit /b %errorlevel%
"%npackd_cl%\ncl" add-repo --url=http://npackd.appspot.com/rep/recent-xml?tag=untested || exit /b %errorlevel%
"%npackd_cl%\ncl" add-repo --url=https://npackd.appspot.com/rep/xml?tag=libs || exit /b %errorlevel%
"%npackd_cl%\ncl" add-repo --url=https://npackd.appspot.com/rep/xml?tag=unstable || exit /b %errorlevel%

"%npackd_cl%\ncl" detect || exit /b %errorlevel%
"%npackd_cl%\ncl" set-install-dir -f "C:\Program Files (x86)" || exit /b %errorlevel%

