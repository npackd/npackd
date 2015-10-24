echo off

start AppVeyorScreenshots.bat

set path=%SystemRoot%;%SystemRoot%\system32;C:\Program Files\AppVeyor\BuildAgent\
@cscript TestUnstableRep.js /password:%password%

