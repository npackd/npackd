set onecmd="%npackd_cl%\npackdcl.exe" path --package=org.xmlsoft.LibXML --versions=[2.4.12,2.4.12]
for /f "usebackq delims=" %%x in (`%%onecmd%%`) do set libxml=%%x

set onecmd="%npackd_cl%\npackdcl.exe" path --package=com.selenic.mercurial.Mercurial64 --versions=[1.8,3)
for /f "usebackq delims=" %%x in (`%%onecmd%%`) do set mercurial=%%x

"%libxml%\bin\xmllint.exe" --noout repository\Rep.xml
if %errorlevel% neq 0 pause & goto :eof

"%libxml%\bin\xmllint.exe" --noout repository\Rep64.xml
if %errorlevel% neq 0 pause & goto :eof

"%libxml%\bin\xmllint.exe" --noout repository\Libs.xml
if %errorlevel% neq 0 pause & goto :eof

"%mercurial%\hg.exe" push
if %errorlevel% neq 0 pause & goto :eof

@echo ============================ SUCCESS ======================================
