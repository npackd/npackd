# Details #

There is also a command line version of Npackd. It is called NpackdCL and you can install it using Npackd. The internal package name is com.googlecode.windows-package-manager.NpackdCL.

NpackdCL follows the versioning scheme of Npackd. It means for example that NpackdCL 1.15.6 uses the same code as Npackd 1.15.6. First number in the version only changes if there are incompatible changes.

Version 1 of the NpackdCL package always has `npackdcl.exe` in the root directory. Execute `npackdcl.exe help` to get the list of command line parameters.

Since 1.19 `ncl.exe` is distributed in the root directory with exactly the same options as `npackdcl.exe` in order to reduce typing.

# List of options for NpackdCL 1.19.13 #

```
NpackdCL 1.19.13 - Npackd command line tool
Usage:
    ncl help
        prints this help
    ncl add (--package=<package> [--version=<version>])+
        installs packages. The newest available version will be installed, 
        if none is specified.
        Short package names can be used here
        (e.g. App instead of com.example.App)
    ncl remove|rm (--package=<package> [--version=<version>])+
           [--end-process=<types>]
        removes packages. The version number may be omitted, 
        if only one is installed.
        Short package names can be used here
        (e.g. App instead of com.example.App)
    ncl update (--package=<package>)+ [--end-process=<types>]
        updates packages by uninstalling the currently installed
        and installing the newest version. 
        Short package names can be used here
        (e.g. App instead of com.example.App)
    ncl list [--status=installed | all] [--bare-format]
        lists package versions sorted by package name and version.
        Please note that since 1.18 only installed package versions
        are listed regardless of the --status switch.
    ncl search [--query=<search terms>] [--status=installed | all]
            [--bare-format]
        full text search. Lists found packages sorted by package name.
        All packages are shown by default.
    ncl info --package=<package> [--version=<version>]
        shows information about the specified package or package version
    ncl path --package=<package> [--versions=<versions>]
        searches for an installed package and prints its location
    ncl add-repo --url=<repository>
        appends a repository to the list
    ncl remove-repo --url=<repository>
        removes a repository from the list
    ncl list-repos
        list currently defined repositories
    ncl detect
        detect packages from the MSI database and software control panel
    ncl check
        checks the installed packages for missing dependencies
    ncl which --file=<file>
        finds the package that owns the specified file or directory
    ncl set-install-dir --file=<directory>
        changes the directory where packages will be installed
    ncl install-dir
        prints the directory where packages will be installed
Options:
    -p, --package        internal package name (e.g. com.example.Editor or just Editor)
    -r, --versions       versions range (e.g. [1.5,2))
    -v, --version        version number (e.g. 1.5.12)
    -u, --url            repository URL (e.g. https://www.example.com/Rep.xml)
    -s, --status         filters package versions by status
    -b, --bare-format    bare format (no heading or summary)
    -q, --query          search terms (e.g. editor)
    -d, --debug          turn on the debug output
    -f, --file           file or directory
    -e, --end-process    list of ways to close running applications (c=close, k=kill). The default value is 'c'.

The process exits with the code unequal to 0 if an error occures.
If the output is redirected, the texts will be encoded as UTF-8.

See https://code.google.com/p/windows-package-manager/wiki/CommandLine for more details.
```

# List of options for NpackdCL 1.18.7 #

```
NpackdCL 1.18.7 - Npackd command line tool
Usage:
    npackdcl help
        prints this help
    npackdcl add (--package=<package> [--version=<version>])+
        installs packages. The newest available version will be installed, 
        if none is specified.
        Short package names can be used here
        (e.g. App instead of com.example.App)
    npackdcl remove|rm (--package=<package> [--version=<version>])+
        removes packages. The version number may be omitted, 
        if only one is installed.
        Short package names can be used here
        (e.g. App instead of com.example.App)
    npackdcl update (--package=<package>)+
        updates packages by uninstalling the currently installed
        and installing the newest version. 
        Short package names can be used here
        (e.g. App instead of com.example.App)
    npackdcl list [--status=installed | all] [--bare-format]
        lists package versions sorted by package name and version.
        Please note that since 1.18 only installed package versions
        are listed regardless of the --status switch.
    npackdcl search [--query=<search terms>] [--status=installed | all]
            [--bare-format]
        full text search. Lists found packages sorted by package name.
        All packages are shown by default.
    npackdcl info --package=<package> [--version=<version>]
        shows information about the specified package or package version
    npackdcl path --package=<package> [--versions=<versions>]
        searches for an installed package and prints its location
    npackdcl add-repo --url=<repository>
        appends a repository to the list
    npackdcl remove-repo --url=<repository>
        removes a repository from the list
    npackdcl list-repos
        list currently defined repositories
    npackdcl detect
        detect packages from the MSI database and software control panel
    npackdcl check
        checks the installed packages for missing dependencies
    npackdcl which --file=<file>
        finds the package that owns the specified file or directory
Options:
    -p, --package        internal package name (e.g. com.example.Editor or just Editor)
    -r, --versions       versions range (e.g. [1.5,2))
    -v, --version        version number (e.g. 1.5.12)
    -u, --url            repository URL (e.g. https://www.example.com/Rep.xml)
    -s, --status         filters package versions by status
    -b, --bare-format    bare format (no heading or summary)
    -q, --query          search terms (e.g. editor)
    -d, --debug          turn on the debug output
    -f, --file           file or directory

The process exits with the code unequal to 0 if an error occures.
If the output is redirected, the texts will be encoded as UTF-8.
```

# Installing software #

The following command would install the latest available version of Python:
`npackdcl.exe add -p Python`

# Removing software #

The following command would remove the newest installed version of CMake:
`npackdcl.exe rm -p CMake`

