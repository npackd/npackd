# Installers #
## NSIS ##

Most (but not all) NSIS-based installers recognize the /S and /D options (silent installation and target directory). If yours does not, all you can do is to ask the installer creator to change this. The /D option is especially important as this is probably the only way to know where the uninst.exe will be created. The uninst.exe can have a different name depending on the installer script. Note that the target directory should not be quoted even if it contains spaces in it. Most installers also require that the target directory is a full path. Note that the parameters use capital letters. Some NSIS un-installation routines delete all files in the installation directory. It is necessary to install packages in a subdirectory in such cases.

.Npackd\Install.bat sample:
```
for /f "delims=" %%x in ('dir /b *.exe') do set setup=%%x
"%setup%" /S /D=%CD% && del /f /q "%setup%"
```

.Npackd\Uninstall.bat sample:
```
uninst.exe /S _?=%CD%
```

## MSI via NpackdInstallerHelper ##
The recommended way to install an .msi package is by using the NpackdInstallerHelper:
```
        <file path=".Npackd\Install.bat">call "%npackdih%\InstallMSI.bat" INSTALLDIR yes</file>
        <dependency package="com.googlecode.windows-package-manager.NpackdInstallerHelper" versions="[1.1, 2)">
            <variable>NIH</variable>
        </dependency>
        <dependency package="com.googlecode.windows-package-manager.NpackdCL" versions="[1.15.7, 2)"/>
```

## MSI without NpackdInstallerHelper ##
/qn is the most "silent" option. TARGETDIR does not always work and always requires an absolute path, but you can uninstall your application either by using the GUID of the installer or the msi file itself as shown below. The ALLUSERS=1 option installs the application for all users.

.Npackd\Install.bat sample:
```
for /f "delims=" %%x in ('dir /b *.msi') do set setup=%%x
copy "%setup%" .Npackd
msiexec.exe /qn /norestart /Lime .Npackd\MSI.log /i "%setup%" TARGETDIR="%CD%" ALLUSERS=1 && del /f /q "%setup%"
set err=%errorlevel%
type .Npackd\MSI.log
rem 3010=restart required
if %err% equ 3010 exit 0
if %err% neq 0 exit %err%
```

.Npackd\Uninstall.bat sample:
```
move .Npackd\*.msi .
for /f "delims=" %%x in ('dir /b *.msi') do set setup=%%x
msiexec.exe /qn /norestart /Lime .Npackd\MSI.log /x "%setup%"
set err=%errorlevel%
type .Npackd\MSI.log
rem 3010=restart required
if %err% equ 3010 exit 0
if %err% neq 0 exit %err%
```

Uninstalling via an MSI GUID:
```
        <file path=".Npackd\Uninstall.bat">MsiExec.exe /qn /norestart /Lime .Npackd\MSI.log /X{1D2C96C3-A3F3-49E7-B839-95279DED837F}
set err=%errorlevel%
type .Npackd\MSI.log
if %err% neq 0 exit %err%
</file>
```

## InnoSetup via NpackdInstallerHelper ##

The recommended way to install an InnoSetup package is by using the NpackdInstallerHelper:
```
        <file path=".Npackd\Install.bat">"%NIH%\InstallInnoSetup.bat"</file>
        <file path=".Npackd\Uninstall.bat">"%NIH%\UninstallInnoSetup.bat" unins000.exe</file>
        <dependency package="com.googlecode.windows-package-manager.NpackdInstallerHelper" versions="[1.3, 2)">
            <variable>NIH</variable>
        </dependency>
```

## Inno Setup ##

.Npackd\Install.bat sample is below. Some Inno Setup un-installation routines delete all files in the installation directory. It is necessary to install packages in a subdirectory in such cases.
```
for /f "delims=" %%x in ('dir /b *.exe') do set setup=%%x
"%setup%" /SP- /VERYSILENT /SUPPRESSMSGBOXES /NOCANCEL /NORESTART /DIR="%CD%" /SAVEINF="%CD%\.Npackd\InnoSetupInfo.ini" /LOG="%CD%\.Npackd\InnoSetupInstall.log" && del /f /q "%setup%"
set err=%errorlevel%
type .Npackd\InnoSetupInstall.log
if %err% neq 0 exit %err%
```

Setup settings can be loaded with the /LOADINF parameter:
```
        <file path=".Npackd\InnoSetupDef.ini">[Setup]
Toolbar=0		
</file>
        <file path=".Npackd\Install.bat">for /f %%x in ('dir /b *.exe') do set setup=%%x
"%setup%" /SP- /VERYSILENT /SUPPRESSMSGBOXES /NOCANCEL /NORESTART /DIR="%CD%" /SAVEINF="%CD%\.Npackd\InnoSetupInfo.ini" /LOG="%CD%\.Npackd\InnoSetupInstall.log" /LOADINF="%CD%\.Npackd\InnoSetupDef.ini"
```

.Npackd\Uninstall.bat sample is below. Note that the log file path must be absolute.
```
unins000.exe /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /LOG="%CD%\.Npackd\InnoSetupUninstall.log"
set err=%errorlevel%
type .Npackd\InnoSetupUninstall.log
if %err% neq 0 exit %err%
```

## InstallShield ##

.Npackd\Install.bat sample:
```
for /f "delims=" %%x in ('dir /b *.exe') do set setup=%%x
"%setup%" /s /v"/qn /norestart /Lime .Npackd\MSI.log AgreeToLicense=YES REBOOT=Suppress ALLUSERS=1 INSTALLDIR="""%CD%""" SYSTRAY=0" && del /f /q "%setup%"
set err=%errorlevel%
type .Npackd\MSI.log
if %err% neq 0 exit %err%
```

.Npackd\Uninstall.bat sample:
```
MsiExec.exe /qn /norestart /Lime .Npackd\MSI.log /X{26A24AE4-039D-4CA4-87B4-2F83216023FF}
set err=%errorlevel%
type .Npackd\MSI.log
if %err% neq 0 exit %err%

```

## QSetup ##

.Npackd\Install.bat sample:
```
for /f "delims=" %%x in ('dir /b *.exe') do set setup=%%x
"%setup%" /Silent /Hide /InstallDir="%CD%" && del /f /q "%setup%"
```

.Npackd\Uninstall.bat sample:
```
un_SuperOrcaSetup_21233.exe /Silent /Hide
```

## ZIP based distributions with an extra directory level with Npackd Installer Helper ##

.Npackd\Install.bat sample:
```
        <file path=".Npackd\Install.bat">for /f "delims=" %%x in ('dir /b aria*') do set name=%%x
"%NIH%\ExtractDir.bat" "%name%"</file>
        <dependency package="com.googlecode.windows-package-manager.NpackdInstallerHelper" versions="[1.3, 2)">
            <variable>NIH</variable>
        </dependency>
```

## ZIP based distributions with an extra directory level ##

.Npackd\Install.bat sample:
```
for /f "delims=" %%x in ('dir /b aria*') do set name=%%x
cd "%name%"        
for /f "delims=" %%a in ('dir /b') do (
  move "%%a" ..
)
cd ..
rmdir "%name%"
```

## Uninstall programs that cannot be forced to use a particular installation directory ##

[NirSoft MyUninstaller](http://www.nirsoft.net/utils/myuninst.html) can be used to uninstall such programs:
```
        <file path=".Npackd\Uninstall.bat">"%myun%\myuninst.exe" /quninstall "Aptana Studio 3"
</file>
        <dependency package="net.nirsoft.MyUninstaller" versions="[1.74, 2)">
            <variable>myun</variable>
        </dependency>
```

## 7z archives ##
```
        <file path=".Npackd\Install.bat">for /f %%x in ('dir /b *.7z') do set setup=%%x
"%sevenzipa%\7za.exe" x "%setup%" &gt; .Npackd\Output.txt &amp;&amp; type .Npackd\Output.txt
</file>
        <dependency package="org.7-zip.SevenZIPA" versions="[9.20, 10)">
            <variable>sevenzipa</variable>
        </dependency>
```

## PATH and similar environment variables ##
It is not recommended to modify PATH and similar variables (see IdealPackage for more details).

The following example shows how npackdcl.exe (Npackd command line interface) can be used to find the best matches for required software during program start:
```
        <file path="gramps.bat">set onecmd="%npackd_cl%\npackdcl.exe" "path" "--package=org.python.Python" "--versions=[2.5,3)"
for /f "usebackq delims=" %%x in (`%%onecmd%%`) do set pythondir=%%x
set onecmd="%npackd_cl%\npackdcl.exe" "path" "--package=org.gtk.GTKPlusBundle" "--versions=[2.22.0,2.23)"
for /f "usebackq delims=" %%x in (`%%onecmd%%`) do set gtkplusbundledir=%%x
set onecmd="%npackd_cl%\npackdcl.exe" "path" "--package=org.pygtk.PyGTK" "--versions=[2.22,2.23)"
for /f "usebackq delims=" %%x in (`%%onecmd%%`) do set pygtkdir=%%x
set onecmd="%npackd_cl%\npackdcl.exe" "path" "--package=org.pygtk.PyGObject" "--versions=[2.26,2.27)"
for /f "usebackq delims=" %%x in (`%%onecmd%%`) do set pygobjectdir=%%x
set onecmd="%npackd_cl%\npackdcl.exe" "path" "--package=org.pygtk.PyCairo" "--versions=[1.8.10,1.8.11)"
for /f "usebackq delims=" %%x in (`%%onecmd%%`) do set pycairodir=%%x

set pythonpath=%pygtkdir%\Lib\site-packages\gtk-2.0;%pygobjectdir%\Lib\site-packages;%pycairodir%\Lib\site-packages
set path=%gtkplusbundledir%\bin
"%pythondir%\python.exe" gramps.py
pause
</file>
```

If you still decide to modify PATH, you could use the CLU package for this:
```
        <file path=".WPM\Install.bat">"%clu%\clu" add-path --path "%CD%"
verify
</file>
        <file path=".WPM\Uninstall.bat">"%clu%\clu" remove-path --path "%CD%"
verify
</file>
        <dependency package="com.googlecode.windows-package-manager.CLU" versions="[1, 2)">
            <variable>clu</variable>
        </dependency>
```

## install4j ##

.Npackd\Install.bat sample:
```
for /f "delims=" %%x in ('dir /b *.exe') do set setup=%%x && del /f /q "%setup%"
"%setup%" -q -dir "%CD%"
```

.Npackd\Uninstall.bat sample:
```
uninstall.exe -q
```

see http://resources.ej-technologies.com/install4j/help/doc/indexRedirect.html?http&&&resources.ej-technologies.com/install4j/help/doc/helptopics/installers/installerModes.html for more details

## Programs that do not support Unicode ##
The easiest way to run programs that do not support Unicode during the installation or uninstallation of a package is to redirect the output to a file:
```
reg add HKLM\SOFTWARE\Classes\Applications\%key%\shell\open /v FriendlyAppName /d "Notepad++ (%version%)" /f > .Npackd\Output.txt && type .Npackd\Output.txt
```

## File associations ##
You can associate an application in your package with a file type using the script shown below. An unique registry key name and title should be used ("net.sourceforge.NotepadPlusPlus-5.8.5" and "Notepad++ (5.8.5)" in the script) to avoid conflicts between packages. As always, a package should not change the default application associated with a file extension. Note that the Windows Explorer does not show "Open With" for .com, .bat and .exe files.
```
        <file path=".Npackd\Install.bat">set package=com.foxitsoftware.FoxitReader
set version=4
set key=%package%-%version%
reg add HKLM\SOFTWARE\Classes\Applications\%key%\shell\open /v FriendlyAppName /d "Notepad++ (%version%)" /f &gt; .Npackd\Output.txt &amp;&amp; type .Npackd\Output.txt
set c=\"%CD%\notepad++.exe\" \"%%1\"
reg add HKLM\SOFTWARE\Classes\Applications\%KEY%\shell\open\command /ve /d "%c%" /f &gt; .Npackd\Output.txt &amp;&amp; type .Npackd\Output.txt
for %%g in (txt log f2k tex sql nfo mak) do reg add HKLM\SOFTWARE\Classes\.%%g\OpenWithList\%key% /f &gt; .Npackd\Output.txt &amp;&amp; type .Npackd\Output.txt
verify
</file>
```

and remove the association as follows:
```
        <file path=".Npackd\Uninstall.bat">set package=com.foxitsoftware.FoxitReader
set version=4
set key=%package%-%version%
reg delete HKLM\SOFTWARE\Classes\Applications\%key% /f &gt; .Npackd\Output.txt &amp;&amp; type .Npackd\Output.txt
for %%g in (txt log f2k tex sql nfo mak) do reg delete HKLM\SOFTWARE\Classes\.%%g\OpenWithList\%key% /f &gt; .Npackd\Output.txt &amp;&amp; type .Npackd\Output.txt
verify
</file>
```

## Shell extensions (.DLL) ##
DLL files registered as shell extensions and used for example to extend the local menu in Windows Explorer cannot be easily uninstalled without logging off all users. Fortunately the DLLs are not really locked in Windows. The files cannot be deleted, but can be moved. The following script can be used to move the DLLs to ".Trash", which the user should clear later manually (after a restart).

```
set package=com.foxitsoftware.FoxitReader
set version=4
set key=%package%-%version%
set trash=..\.Trash\%key%_%random%
mkdir %trash%
move ClamWin\bin\ExpShell.dll %trash%
verify
```

## Multiple shortcuts in a sub-directory ##
.Npackd\Install.bat
```
cscript .Npackd\CreateShortcuts.js
```

.Npackd\CreateShortcuts.js
```
var shell = new ActiveXObject("WScript.Shell"); 
var dir = shell.CurrentDirectory;
var menu = shell.SpecialFolders("AllUsersStartMenu"); 

var fso = new ActiveXObject("Scripting.FileSystemObject");
if (!fso.FolderExists(menu + "\\D"))
    fso.CreateFolder(menu + "\\D");

var s = shell.CreateShortcut(menu + "\\D\\D2 32-bit Command Prompt.lnk"); 
s.TargetPath = "%comspec%";
s.Arguments = "/K \"" + dir + "\\dmd2vars32.bat\"";
s.Save(); 

s = shell.CreateShortcut(menu + "\\D\\D2 Documentation.lnk"); 
s.TargetPath = dir + "\\html\\d\\index.html";
s.Save(); 

s = shell.CreateShortcut(menu + "\\D\\D2 HTML Documentatio.lnk"); 
s.TargetPath = dir + "\\windows\\bin\\d.chm";
s.Save(); 
```

.Npackd\Unnstall.bat
```
cscript .Npackd\DeleteShortcuts.js
```

.Npackd\DeleteShortcuts.js
```
var shell = new ActiveXObject("WScript.Shell"); 
var menu = shell.SpecialFolders("AllUsersStartMenu"); 
var fso = new ActiveXObject("Scripting.FileSystemObject");

function deleteIfExists(f) {
    if (fso.FileExists(f))
        fso.DeleteFile(f, true);
}

deleteIfExists(menu + "\\D\\D2 32-bit Command Prompt.lnk");
deleteIfExists(menu + "\\D\\D2 Documentation.lnk");
deleteIfExists(menu + "\\D\\D2 HTML Documentatio.lnk");

if (fso.FolderExists(menu + "\\D")) {
    var f = fso.GetFolder(menu + "\\D");
    if (new Enumerator(f.SubFolders).atEnd() && new Enumerator(f.Files).atEnd())
        f.Delete(true);
}
```

## Changing a value in the registry ##
.Npackd\Unnstall.bat
```
cscript .Npackd\WriteRegistry.js
```

.Npackd\WriteRegistry.js
```
var wsh = WScript.CreateObject("WScript.Shell");
var val = wsh.RegWrite("HKEY_LOCAL_MACHINE\\SOFTWARE\\MyCompany\\MySoftware\\Settings\\",
    wsh.CurrentDirectory + "\\MyApp.exe");
```

## Testing for errors ##
```
if %errorlevel% neq 0 exit /b %errorlevel%
```
or
```
command || exit /b %errorlevel%
```

## Testing for 64-bit Windows ##
```
if "%ProgramFiles(x86)%" NEQ "" goto x64
```

## Certificate installation ##
```
certutil.exe -addstore -f "TrustedPublisher" "%CD%\oracle-vbox.cer" > .Npackd\Output.txt && type .Npackd\Output.txt
```

## InstallJammer ##
.Npackd\Install.bat sample:
```
for /f "delims=" %%x in ('dir /b *.exe') do set setup=%%x
"%setup%" /mode console /prefix "%CD%" /save-response-file .Npackd\InstallJammer.properties && del /f /q "%setup%"
```

.Npackd\Uninstall.bat sample:
```
uninstall.exe /mode console
```

Use "install.exe /help" to get the list of available options. The complete guide is here:
http://www.installjammer.com/docs/InstallJammerUserGuide.pdf . A complete example: http://npackd.appspot.com/p/com.google.codeexchangerxml/3.3.1


## Burn Installer ##
.Npackd\Install.bat sample:
```
for /f "delims=" %%x in ('dir /b *.exe') do set setup=%%x
"%setup%" /install /quiet /norestart /log .Npackd\Setup.log && move "%setup%" .Npackd\Setup.exe
```

.Npackd\Uninstall.bat sample:
```
.Npackd\Setup.exe /uninstall /quiet /norestart /log .Npackd\Setup.log
```

More details:
http://windows-installer-xml-wix-toolset.687559.n2.nabble.com/Running-Burn-driven-installer-in-quiet-mode-command-line-parameters-td5913001.html

## Advanced Installer ##

/qn is the most "silent" option. TARGETDIR does not always work and always requires an absolute path, but you can uninstall your application either by using the GUID of the installer or the msi file itself as shown below. The ALLUSERS=1 option installs the application for all users.

.Npackd\Install.bat sample:
```
for /f "delims=" %%x in ('dir /b *.msi') do set setup=%%x
"%setup%" /exenoui /exelog .Npackd\Setup.log /qn /norestart ALLUSERS=1 INSTALLLOCATION="%CD%" && del /f /q "%setup%"
set err=%errorlevel%
type .Npackd\MSI.log
rem 3010=restart required
if %err% equ 3010 exit 0
if %err% neq 0 exit %err%
```
