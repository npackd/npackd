# Build 1.18 #
See wpmcpp\BUILD.txt

# Build 1.17 #
Use Qt Creator to build manually

# Build 1.16 #
See comments in Build.py

# Build all versions before (not including) 1.16 #
  * Qt SDK 2010.03
  * MinGW does not provide msi.h (Microsoft Installer interface) and the corresponding library. msi.h was taken from the MinGW-W64 project and is committed together with libmsi.a in the Mercurial repository. To re-create the libmsi.a file do the following:
    * download mingw-w32-bin\_i686-mingw\_20100914\_sezero.zip
    * run gendef C:\Windows\SysWOW64\msi.dll
    * run dlltool -D C:\Windows\SysWOW64\msi.dll -V -l libmsi.a
  * download quazip from http://sourceforge.net/projects/quazip/ and build it
