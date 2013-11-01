rem This script installs an .exe file created using the InnoSetup installer
rem as an Npackd package. There must be only one .exe file in the current 
rem directory. The .exe file will be deleted.
rem 
rem Script parameters: none
mkdir .Npackd

for /f "delims=" %%x in ('dir /b *.exe') do set setup=%%x
"%setup%" /SP- /VERYSILENT /SUPPRESSMSGBOXES /NOCANCEL /NORESTART /DIR="%CD%" /SAVEINF="%CD%\.Npackd\InnoSetupInfo.ini" /LOG="%CD%\.Npackd\InnoSetupInstall.log" && del /f /q "%setup%"
set err=%errorlevel%
type .Npackd\InnoSetupInstall.log
if %err% neq 0 exit %err%

