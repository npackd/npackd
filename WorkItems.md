## In no particular order ##
You can use the list of activities below to find something appropriate if you would like to contribute.

### Developing software ###
  * GUI. C++, MinGW-w64, Qt skills are required here.
  * command line package manager. C++, MinGW-w64, Qt skills are required here.
  * creating installations. C++, MinGW-w64, Qt skills are required here.
  * testing. There are no automated tests or test plans yet. The quality seems OK, maybe later. But testing on different operating systems is very appreciated.

### Updating repository ###
  * finding new applications/packages
  * filling the license information for packages
  * describing new packages. Windows command line/cmd.exe skills are required here.
> Example of a package description:
```
<package name="com.tracker-software.PDFXChangeViewer64">
    <title>PDF-XChange Viewer/64 bit</title>
    <url>http://www.tracker-software.com/product/pdf-xchange-viewer</url>
    <description>PDF viewer</description>
</package>
```
> Example of a package version description (the most simple example):
```
<version name="3.6.2" package="com.xxcopy.XXCopy">
    <url>http://xxcopy.com/download/xxfw3062.zip</url>
    <sha1>7cb2ff93b3e7b9cf8040d7ae1a1d598d2b4fa0bc</sha1>
</version>
```
> See InstallationScripts for more details.
  * describing and testing new versions of already available packages. Windows command line/cmd.exe skills are required here. See InstallationScripts for more details.

### Issue tracking ###
  * file issues if you find a bug or think something can be improved in any of the above items