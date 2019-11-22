echo on

start "Screenshots" cmd.exe /c AppVeyorScreenshots.bat

set path=%SystemRoot%;%SystemRoot%\system32;C:\Program Files\AppVeyor\BuildAgent\
go run TestUnstableRep.go

