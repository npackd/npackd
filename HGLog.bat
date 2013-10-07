rem this script creates a list of changes that can be used to create a
rem what's new page like this one:
rem http://code.google.com/p/windows-package-manager/wiki/WhatIsNewInNpackd1_16
hg log -r 2497:default --template "  * {desc}\n" | clip
