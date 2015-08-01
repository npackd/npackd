echo off

start AppVeyorScreenshots.bat
@cscript TestUnstableRep.js /password:%password%

