echo on

set n=5
:l
"C:\Program Files (x86)\NirCmd_64_bit\nircmdc.exe" cmdwait 300000 savescreenshot screen%n%.png
appveyor PushArtifact screen%n%.png || exit /b %errorlevel%
set /a "n = n + 5"
if %n% equ 35 goto :eof
goto l

