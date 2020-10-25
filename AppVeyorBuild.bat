echo on

if [%what%]==[test-packages] start "Screenshots" cmd.exe /c AppVeyorScreenshots.bat

rem set path=%SystemRoot%;%SystemRoot%\system32;C:\Program Files\AppVeyor\BuildAgent\
go run TestUnstableRep.go TestUnstableRep_windows.go -command %what% -password %password% -github-token %github_token%

