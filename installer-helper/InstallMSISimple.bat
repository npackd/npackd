rem This script installs an .msi file as Npackd package
rem There must be only one .msi file in the current directory. Additionally it
rem also installs the corresponding Uninstall.bat to remove the MSI package
rem later.
rem 
rem Script parameters:
rem %1 - name of the MSI parameter like INSTALLDIR that changes the installation
rem directory. Other often used values are TARGETDIR and INSTALLLOCATION
for /f "delims=" %%x in ('dir /b *.msi') do set setup=%%x
mkdir .Npackd
move "%setup%" .Npackd
set err=%errorlevel%
if %err% neq 0 exit %err%

rem MSIFASTINSTALL: http://msdn.microsoft.com/en-us/library/dd408005%28v=VS.85%29.aspx
msiexec.exe /qn /norestart /Lime .Npackd\InstallMSI.log /i "%setup%" %1="%CD%" ALLUSERS=1 MSIFASTINSTALL=7
set err=%errorlevel%
type .Npackd\InstallMSI.log

rem list of errors: http://msdn.microsoft.com/en-us/library/windows/desktop/aa368542(v=vs.85).aspx
rem 3010=restart required
if %err% equ 3010 goto cont
if %err% neq 0 exit %err%

:cont
copy %~dp0\UninstallMSI.bat .Npackd\Uninstall.bat
