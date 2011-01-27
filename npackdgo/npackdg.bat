set GOROOT=C:\WPM\com.googlecode.gomingw.GoMinGW-2011.1.20
set GOOS=windows
set GOARCH=386
"%goroot%\bin\8g" npackdg.go
"%goroot%\bin\8l" npackdg.8
8.out.exe
pause
