rem This script installs an .msi file as Npackd package
rem There must be only one .msi file in the current directory. 
rem 
rem Script parameters:
rem %1 - name of the MSI parameter like INSTALLDIR that changes the installation
rem directory. Other often used values are TARGETDIR and INSTALLLOCATION
rem %2 - optional parameter. If this value is "yes", the default uninstall
rem script Uninstall.bat is also created to remove the MSI package later.
for /f "delims=" %%x in ('dir /b *.msi') do set setup=%%x
mkdir .Npackd
move "%setup%" .Npackd
set err=%errorlevel%
if %err% neq 0 exit %err%

rem MSIFASTINSTALL: http://msdn.microsoft.com/en-us/library/dd408005%28v=VS.85%29.aspx
msiexec.exe /qn /norestart /Lime .Npackd\InstallMSI.log /i ".Npackd\%setup%" %1="%CD%" ALLUSERS=1 MSIFASTINSTALL=7
set err=%errorlevel%
type .Npackd\InstallMSI.log

rem list of errors: http://msdn.microsoft.com/en-us/library/windows/desktop/aa368542(v=vs.85).aspx
rem 3010=restart required
if %err% equ 3010 goto cont
if %err% neq 0 exit %err%

:cont
if "%2" equ "yes" copy "%~dp0\UninstallMSIDefault.bat" .Npackd\Uninstall.bat
