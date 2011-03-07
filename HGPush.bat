C:\WPM\LibXML-2.4.12\bin\xmllint.exe --noout repository\Rep.xml
if %errorlevel% neq 0 pause & goto eof

C:\WPM\LibXML-2.4.12\bin\xmllint.exe --noout repository\Rep64.xml
if %errorlevel% neq 0 pause & goto eof

call hg push

pause
