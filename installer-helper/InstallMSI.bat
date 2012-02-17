rem This script installs an .msi file as Npackd package
rem There must be only one .msi file in the current directory
rem 
rem Script parameters:
rem %1 - name of the MSI parameter like INSTALLDIR that changes the installation
rem directory. Other often used values are TARGETDIR and INSTALLLOCATION
for /f "delims=" %%x in ('dir /b *.msi') do set setup=%%x
mkdir .Npackd
copy "%setup%" .Npackd
set err=%errorlevel%
if %err% neq 0 exit %err%
msiexec.exe /qn /norestart /Lime .Npackd\InstallMSI.log /i "%setup%" %1="%CD%" ALLUSERS=1
set err=%errorlevel%
type .Npackd\InstallMSI.log
rem 3010=restart required
if %err% equ 3010 exit 0
if %err% neq 0 exit %err%
