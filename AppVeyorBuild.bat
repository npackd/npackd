echo off

start AppVeyorScreenshots.bat

set path=%SystemRoot%;%SystemRoot%\system32
@cscript TestUnstableRep.js /password:%password%

