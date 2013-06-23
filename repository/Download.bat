set onecmd="%npackd_cl%\npackdcl.exe" path --package=se.haxx.curl.CURL64 --versions=[7,8)
for /f "usebackq delims=" %%x in (`%%onecmd%%`) do set curl=%%x
if "%curl%" == "" pause & goto :eof

set onecmd="%npackd_cl%\npackdcl.exe" path --package=org.mingw.MSYS --versions=[2011,2020)
for /f "usebackq delims=" %%x in (`%%onecmd%%`) do set msys=%%x
if "%curl%" == "" pause & goto :eof

"%curl%\curl.exe" -o Rep.xml http://npackd.appspot.com/rep/xml?tag=stable
if %errorlevel% neq 0 pause & goto :eof

"%curl%\curl.exe" -o Rep64.xml http://npackd.appspot.com/rep/xml?tag=stable64
if %errorlevel% neq 0 pause & goto :eof

"%curl%\curl.exe" -o Libs.xml http://npackd.appspot.com/rep/xml?tag=libs 
if %errorlevel% neq 0 pause & goto :eof

"%curl%\curl.exe" -o RepUnstable.xml http://npackd.appspot.com/rep/xml?tag=unstable
if %errorlevel% neq 0 pause & goto :eof

"%msys%\bin\unix2dos" Rep.xml Rep64.xml Libs.xml RepUnstable.xml

@echo ============================ SUCCESS ======================================
