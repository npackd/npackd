## Introduction ##

A repository consists of a single XML file that can be placed on a HTTP server. The file contains package and package version descriptions and references the packages using URLs. This makes at possible to create a software compilation repository with packages from different software vendors.

Since version 3.3 of this specification (Npackd version 1.20) it is also possible to place the repository XML named "Rep.xml" in a ZIP file.

Npackd operates on the list of repositories defined for a particular computer. All definitions in these repositories are combined in the order of the appearance. This means that definitions from earlier defined repositories may hide the definitions from the repositories that stay further in the list. Different parts of a repository like packages, package versions or licenses are referenced by their identifiers. A package version may reference a package that none of the repositories defines. In this case a dummy package with the specified identifier will be automatically created. The same happens if a package references a non-existing license definition

Remark:
  * Npackd 1.19 and later can theoretically download 16 EiB big packages (tested with 13 GiB) and supports .zip files bigger than 4 GiB (zip64 file format)
  * Npackd 1.17 can theoretically download 16 EiB big packages (tested with 8 GiB), but cannot extract .zip files bigger than 4 GiB
  * Npackd 1.16 can theoretically download 16 EiB big packages (tested with 8 GiB), but does not show the progress during the download of files bigger than 2 GiB and cannot extract .zip files bigger than 4 GiB
  * Npackd before 1.16 only can download packages smaller than 2 GiB in size.

## Details ##

The root element should be named **root** and can contain **package** and **version** tags:

```
<root>
    <!-- 
        (since 2) specification version (1.0 if missing). The first number denotes
        incompatible changes (e.g. 2 is incompatible with 1), the second number
        denotes compatible changes. 
        Npackd 1.13 and earlier versions implement the specification in the version 1. 
        Npackd 1.14 implements the version 2 of the specification.
        Npackd 1.15 implements the version 2.1 of the specification.
        Npackd 1.16 implements the version 3 of the specification.
        Npackd 1.18 implements the version 3.1 of the specification.
        Npackd 1.19 implements the version 3.2 of the specification.
        Npackd 1.20 implements the version 3.3 of the specification.
    -->
    <spec-version>3.3</spec-version>

    <license ...>
        ....
    </license>
    <license ...>
        ....
    </license>
    ...
    <package ...>
        ...
    </package> 
    <package ...> 
        ...
    </package> 
    ...
    <version ...> 
        ...
    </version> 
    <version ...> 
        ...
    </version> 
    ...
</root>
```

### IDs ###
IDs are used at different places in the repository XML: as full internal package
names, internal license names or package tags. Each ID is prefixed with a
reversed domain name of the software vendor.
The last part of the ID is the name of the item in camel case.
IDs cannot contain start or end with a dot or contain two dots consecutively.
The parts of an ID cannot start or end with a dash or contain two dashes
consecutively. Valid characters for the package name parts are:
'-', '0'...'9', 'A'...'Z', `'_'`, 'a'..'z' and other Unicode letters.

### The license element ###

```
<!--
    the *name* attribute is mandatory and contains the ID of the license.     
-->
<license name="org.gnu.GPLv3">
    <!-- (since 2.1/Npackd 1.15) optional one-line title of the license -->
    <title>GPLv3</title>

    <!-- (since 2.1/Npackd 1.15) an optional web page URL for the license description -->
    <url>http://www.gnu.org/licenses/gpl.html</url>
</license>
```

### The package element ###

The **name** attribute is mandatory and is an ID.

```
<package name="com.example.buggy-text-editor">
    <!-- (since 1/Npackd 1.7.0) optional one-line name of the Application -->
    <title>The Buggy Editor</title>

    <!-- 
        (since 1/Npackd 1.9.0) an optional web page URL for package 
        description. The same value can be supplied using
        the <link rel="homepage" href="..."/> tag.
     -->
    <url>http://www.example.com/BuggyTextEditor.html</url>

    <!-- (since 1/Npackd 1.9.0) an optional description (multiline) -->
    <description>the most buggy editor</description>

    <!--
        (since 2) optional URL for the package icon. Only .png icons are supported.
        The usual size is 32x32 pixels. The same value can be supplied using
        the <link rel="icon" href="..."/> tag.
    -->
    <icon>http://www.example.com/BuggyTextEditor/icon.png</icon>

    <!-- (since 2) optional license name -->
    <license>org.gnu.GPLv3</license>

    <!-- 
        (since 3.1/Npackd 1.18) a category. The possible sub-categories should 
        be separated by
        a slash "/". Multiple categories are allowed for the same package.
        See also the list of recommended categories below.
        Implementation note: Npackd 1.18 only supports one category element per package
        and only up to 2 category levels.
    -->
    <category>Text/Editor</category>

    <!-- 
        (since 3.3/Npackd 1.20) this tag is optional. The tag "link" creates a 
        link from this package to another resource on the Internet. Npackd only
        supports "http:" and "https:" URL schemes. The valid 
        values for the attribute "rel" are:
        - "homepage"  an optional web page for package description. It is 
            recommended to use the "url" tag instead for backward compatibility. 
            Npackd only supports one home page per package.
        - "icon" package icon. Only .png icons are supported. The usual size is
            32x32 pixels. It is recommended to use the "icon"
            tag instead for backward compatibility. Implementation note: Npackd
            only can use one icon in the UI.
        - "changelog" web page for the package change log. Npackd only supports
            one change log per package.
        - "screenshot" PNG screen shot. Multiple screen shots are allowed.
        The attribute "href" is mandatory and should contain a valid URL to the
        resource.
    -->
    <link rel="homepage" href="http://www.example.com/BuggyTextEditor"/>
    <link rel="icon" href="http://www.example.com/BuggyTextEditor/icon.png"/>
    <link rel="changelog" href="http://www.example.com/BuggyTextEditor/ChangeLog.html"/>
    <link rel="screenshot" href="http://www.example.com/BuggyTextEditor/Screen.png"/>
</package>
```

### Categories ###
List of recommended categories:
  * Communications: communications (social media, messaging, call management, etc.)
  * Dev: software development (libraries, IDEs, SDKs, etc.)
  * Education: education (educating, teaching, learning)
  * Finance: finance (managing personal finances, banking, payments, ATM finders, financial news, insurance, taxes, portfolio, trading, tip calculators)
  * Games: games
  * Music: music (creating or listening to)
  * News: news (newspapers, news aggregators, magazines, blogging, weather, etc.)
  * Photo: photography (image processing and sharing, cameras, photo management, etc.)
  * Productivity: productivity (text editors, to do list, printing, scheduling, organization and communication, calendars)
  * Security: security (antivirus, anti-malware, and firewalls)
  * Text: text (text editors, LaTeX, ViM, Emacs, word processors, etc.)
  * Tools: tools (applications that help users to accomplish a specific task, such as calculators)
  * Video: video (creating or viewing videos, subscription movie services, remote controls, etc.)

### Package naming rules ###

Since April 2013 the recommended naming scheme was changed from camel cased
identifiers like QuaZIPSource to all lowercase separated by dashes like
quazip-source. Additionally the default repositories hosted at
http://code.google.com/p/windows-package-manager started to use empty domain
prefix which is now reserved for the default repositories.

For the most open source projects the domain already contains the name of the project and the package name will contain this twice.

Example: com.blender.blender is right, com.blender is wrong.

A package name should not contain the name of the vendor.

Example: com.microsoft.windows is right, but com.microsoft.microsoftWindows is wrong.

A package name should not contain the "www" suffix.

Example: com.blender.blender is right, com.blender.www.blender is wrong.

See [PackageCPPLibraries](PackageCPPLibraries.md) for how C and C++ libraries should be packaged.

### The version element ###

Each package version tag must have a **name** attribute for the version number and a **package** attribute that references a previously defined package. Only numeric version numbers are supported. You will have to change "1.2b" for example to "1.2.2" or similar. For beta or alpha versions the "999" prefix can be used. For example "2.55beta" can be expressed as "2.54.999.1". An attribute **type** can have one of the two values **one-file** or **zip** (default value).

Packages of type **zip** are normal ZIP files with arbitrary structure.  It is not recommended to have a top-level directory in ZIP packages as this only creates additional sub-directory on the hard drive, but it is permitted.

Packages of the type **one-file** can be files of any type. They are typically used to reference .msi or .exe files, but can also be used to download a database or similar data that consists of one file.

**(since 1)** the files ".WPM\Install.bat" and ".WPM\Uninstall.bat" in the package directory or .zip file will be called to make additional changes to the system if necessary (see also the tag "file" below). The batch files will be executed from the package directory (e.g. C:\Program Files (x86)\Npackd\com.activestate.ActivePerl-5.10.1.1007)

**(since 2)** the files ".Npackd\Install.bat" and ".Npackd\Uninstall.bat" in  the package directory or .zip file will be called to make additional changes to the system if necessary (see also the tag "file" below). If ".Npackd\Install.bat" exists, it is used instead of ".WPM\Install.bat". The same applies to ".WPM\Uninstall.bat". The batch files will be executed from the package directory (e.g. C:\Program Files (x86)\Npackd\com.activestate.ActivePerl-5.10.1.1007)

**(since 2.1)** a system-wide variable with the name NPACKD\_CL is always defined and points to the newest installed version of com.googlecode.windows-package-manager.NpackdCL. You can use npackdcl.exe to find other packages. During the execution of .Npackd\Install.bat and .Npackd\Uninstall.bat scripts the environment variables NPACKD\_PACKAGE\_NAME and NPACKD\_PACKAGE\_VERSION are defined and contain the full package name and version of the package currently being installed or uninstalled.

**(since 3)** cmd.exe is used instead of running the .bat file directly. The latter uses the command line processor associated with .bat file, which is in most cases cmd.exe. Previous versions also always ran 32-bit version of cmd.exe. Since Npackd 1.16 64-bit version is choosen on 64-bit systems. The exact parameters used are:
cmd.exe /U /E:ON /V:OFF /C ""<path and name of a .bat file>""

**(since 3.1)**
The environment variable NPACKD\_PACKAGE\_BINARY is defined during the installation for package versions of type **one-file** and contains the full path and file name to the downloaded binary. Example: C:\Program Files\MyPackage\MyPackageInstaller.exe

```
<version name="5.10.1.1007" package="com.activestate.active-perl"
        type="one-file"> 
    <!--
        The nested tag *important-file* is used to denote important files in a package. 
        Shortcuts in the Windows start menu will be automatically created/deleted 
        if a package is installed/unistalled. The *path* and *title* attributes are 
        used to choose a file from the package and to give it an appropriate title.
        Some .exe or .msi based installations override shortcuts in the start menu
        from a different version of the same package. That is why it is sometimes
        necessary to use this tag even for such installations.
    -->
    <important-file path="ActivePerl-5.10.1.1007-MSWin32-x86.msi"
            title="Installer"/> 

    <!-- 
        (since 1/Npackd 1.10) additional files can be placed in the package directory. The whole file content 
        is supplied between in a file tag. The path attribute is mandatory and contains a
        relative path where the file should be stored. This can be used to place scripts
        for installation and uninstallation for packages that do not follow the 
        package format.

        (since Npackd 1.16) the contents is saved in UTF-8 instead
        of the default system encoding. Please note that .bat files 
        are always encoded using the default platform encoding
        Unicode is not supported in batch files by cmd.exe, 
        not even UTF-16. This means that you should only use ASCII 
        characters in batch files. UTF-8 has the same internal 
        representation as one-byte-per-character Windows encodings 
        for ASCII characters 0 to 127. Nonetheless cmd.exe is started
        with the /U parameter and all input and output for the internal
        commands will be done using UTF-16.

        (since 3.2/Npackd 1.19) the file .Npackd\Stop.bat may be included and
        will be called to stop a running package version. If the return code
        of this script is not equal to zero the package removal operation fails.
        If this file does not exist, the program will be stopped using default 
        available methods like closing its windows or killing the process.
    -->
    <file path=".Npackd\Install.bat">msiexec.exe /I ActivePerl-5.10.1.1007-MSWin32-x86.msi</file>
    <file path=".Npackd\Uninstall.bat">msiexec.exe /uninstall ActivePerl-5.10.1.1007-MSWin32-x86.msi</file>

    <!-- 
        URL where the package can be downloaded (http: or https:).
        (since 2.1) the tag is not mandatory. Package version 
        definitions without a valid URL can only be used for detection. 
    -->
    <url>http://downloads.activestate.com/ActivePerl/releases/5.10.1.1007/ActivePerl-5.10.1.1007-MSWin32-x86.msi</url> 

    <!-- (since 1/Npackd 1.13) SHA1 hash for the download file (optional) -->
    <sha1>68ac906495480a3404beee4874ed853a037a7a8f</sha1>

    <!-- 
        (since 3.2/Npackd 1.19) SHA-256 hash for the download file (optional).
        Please note that defining both *sha1* tag and *hash-sum* tag is not
        allowed. The type attribute may contain either *SHA-1* or *SHA-256* and
        may be missing. The default value is *SHA-256*.
    -->
    <hash-sum type="SHA-256">d32b568cd1b96d459e7291ebf4b25d007f275c9f13149beeb782fac0716613f8</hash-sum>

    <!-- 
        (since 2) a package dependency. The format is ("[" | "(") <Version> "," <Version> (")" | "]").
        The square brackets [ and ] are used for inclusive bounds, round brackets ( and ) are used for
        non-inclusive bounds. The package with the name com.microsoft.Windows is
        used to express compatible Windows versions. The dependency below is equivalent to
        [Windows 2000...Windows 7). Since Npackd 1.15 also
        com.microsoft.Windows32 (only on 32-bit Windows) and
        com.microsoft.Windows64 (only on 64-bit Windows) are defined.
        (since 3) an optional "variable" tag can be used to compute the installation
        path of the dependency and store it in the environment variable
        with the specified name. This variable will only be available
        in installation and un-installation scripts in this package
        version.
    -->
    <dependency package="com.microsoft.Windows" versions="[5.00.2195, 6.1)">
        <variable>WINDOWS_PATH</variable>
    </dependency>

    <!--
        (since 2.1) optional MSI product GUID for package
        detection
    -->
    <detect-msi>{1D2C96C3-A3F3-49E7-B839-95279DED837F}</detect-msi>

    <!--
        (since 2.1) package detection by SHA1 check sum of a file. If
        multiple detect-file tags are present, all files must match 
        for the package to be detected.
    -->
    <detect-file>
        <!-- relative file path -->
        <path>ide\modules\org-netbeans-core-ide.jar</path>

        <!-- SHA1 of the file -->
        <sha1>8D244BE2B690093283138698AF5669A6861E5E20</sha1>
    </detect-file>
</version> 
```