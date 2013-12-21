rem This script installs and then removes an Npackd package. It prints a success
rem message if everything can be performed without an error.
rem 
rem Script parameters:
rem %1 - name of the package that should be installed (e.g. "Firefox")
rem %2 - version number (e.g. "14")
rem
rem this script requires at least NpackdCL 1.18
"%NPACKD_CL%\npackdcl.exe" detect
if %errorlevel% neq 0 exit /b %errorlevel%

"%NPACKD_CL%\npackdcl.exe" add --package=%1 --version=%2
if %errorlevel% neq 0 exit /b %errorlevel%

set onecmd="%NPACKD_CL%\npackdcl.exe" "path" "--package=%1" "--versions=[%2, %2]"
for /f "usebackq delims=" %%x in (`%%onecmd%%`) do set D=%%x
if %errorlevel% neq 0 exit /b %errorlevel%

dir "%D%"

"%NPACKD_CL%\npackdcl.exe" remove --package=%1 --version=%2
if %errorlevel% neq 0 exit /b %errorlevel%

@echo ========================== SUCCESS ========================================

