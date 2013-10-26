rem This script removes a program installed by an InnoSetup installer.
rem 
rem Script parameters: name of the uninstall program. If the parameter is empty
rem unins000.exe is used
mkdir .Npackd

set name=%1
if "%name%" equ "" set name=unins000.exe

"%name%" /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /LOG="%CD%\.Npackd\InnoSetupUninstall.log"
set err=%errorlevel%
type .Npackd\InnoSetupUninstall.log
if %err% neq 0 exit %err%

