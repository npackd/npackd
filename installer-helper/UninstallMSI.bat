rem This script uninstalls an .msi file as Npackd package previosly installed
rem by InstallMSI.bat.
move .Npackd\*.msi .
for /f "delims=" %%x in ('dir /b *.msi') do set setup=%%x

rem MSIFASTINSTALL: http://msdn.microsoft.com/en-us/library/dd408005%28v=VS.85%29.aspx
msiexec.exe /qn /norestart /Lime .Npackd\UninstallMSI.log /x "%setup%" MSIFASTINSTALL=7
set err=%errorlevel%
type .Npackd\UninstallMSI.log
rem 3010=restart required
if %err% equ 3010 exit 0
if %err% neq 0 exit %err%
