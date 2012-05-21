if "%npackd_cl%" equ "" set npackd_cl=..\com.googlecode.windows-package-manager.NpackdCL-1
set onecmd="%npackd_cl%\npackdcl.exe" "path" "--package=com.googlecode.windows-package-manager.NpackdInstallerHelper" "--versions=[1, 1]"
for /f "usebackq delims=" %%x in (`%%onecmd%%`) do set npackdih=%%x
call "%npackdih%\UninstallMSI.bat"
