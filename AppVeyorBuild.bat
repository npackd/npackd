echo on

rem set path=%SystemRoot%;%SystemRoot%\system32;C:\Program Files\AppVeyor\BuildAgent\
go run TestUnstableRep.go TestUnstableRep_windows.go -command %what%

