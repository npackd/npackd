echo on

set n=5
:l
"C:\Program Files (x86)\NirCmd_64_bit\nircmdc.exe" cmdwait 300000 savescreenshot screen%n%.png
appveyor PushArtifact screen%n%.png
set /a "n = n + 5"
if %n% equ 55 goto :eof
goto l

