# 1.19 #

## Npackd ##
  * stop running programs if necessary
  * support for .zip files bigger than 4 GiB (zip64 file format)
  * check the downloaded binaries with the installed antivirus
  * fill the table immediately after the start, asynchronous database update
  * better package detection (less duplicates)
  * better warnings if something will be removed or installed
  * auto-update
  * load MSI and control panel icons
  * store/load the list of previously used installation directories
  * "Open folder" action to show the package directory in Windows Explorer
  * more tooltips
  * npackdg.exe "start-newest" command to start the newest version of Npackd GUI
  * npackdg.exe "add" to add a package with a GUI progress dialog
  * npackdg.exe "remove" to remove a package with a GUI progress dialog
  * support for SHA-256 cryptographic hash sums
  * bugfix: .lnk descriptions can only contain one line
  * use Qt 5.2.1, MinGW-w64 4.8.2, QuaZIP 0.6.2
  * better installation location detection for MSI packages
  * save the list of repositories in the database
  * do not use script mode while parsing the URLs
  * show text files associated with a package version
  * change the home page to https://npackd.appspot.com
  * reload tabs if the repository was reloaded instead of closing them
  * bugfix: ROLLBACK is used outside of a transaction
  * bugfix: finish() SQL queries

## NpackdCL ##
  * distribute "ncl.exe" with the same functionality as "npackdcl.exe" for less typing
  * auto-update
  * set-install-dir/get-install-dir
  * faster commands that do not install or remove packages
  * bugfix: do not move NpackdCL installation path if started from another directory
  * show text files associated with a package version in the command line
  * bugfix for [Issue 353](https://code.google.com/p/windows-package-manager/issues/detail?id=353): "Program Files (x86)"/"Program Files" conventions not respected.
  * do not modify PATH in the NpackdCL installer

## Repository format ##
  * the file .Npackd\Stop.bat may be included and will be called to stop a running package version.
  * support for the SHA-256 hash sum

# 1.18 #

## Npackd ##
  * i18n: French translation by baptist.benoist, German translation by nipsky, Russian translation by knockdowncore, Spanish translation by www.jon99
  * categories
  * faster filling of the table (only the visible cells are filled)
  * download only the necessary icons
  * store and load main window size, toolbar position and column widths
  * use the Windows HTTP cache for the repositories
  * show a "wait" icon while loading the real package icon
  * support for bigger repositories (store the data in the SQlite database)
  * smaller size (static linking, use the zlib from Qt)
  * provide progress information for long running package detection
  * radio buttons for different package types instead of a combobox (filtering)
  * show the actual error message if a shortcut cannot be created
  * search for all possible matches while searching for a dependency
  * show URL for file download errors
  * lock the .Npackd\Uninstall.bat script during the installation. This prevents it from being deleted by an uninstaller.
  * save the Install.bat/Uninstall.bat output in a file
  * handle Windows registry write errors
  * show the detection info on the package version details tab
  * bugfix: directories under NpackdDetected are created over and over again
  * bugfix for the [issue 260](https://code.google.com/p/windows-package-manager/issues/detail?id=260): All Packages are deleted from registry on Npackd start
  * bugfix for the [issue 220](https://code.google.com/p/windows-package-manager/issues/detail?id=220): Applications hangs at 'Detecting MSI packages'

## Repository format ##
  * categories
  * NPACKD\_PACKAGE\_BINARY environment variable is automatically set during the install script execution to the name of the downloaded file

## NpackdCL ##
  * remove, update or install several packages at once
  * faster "npackdcl path" (< 5 ms)
  * "npackdcl list-repos" to list of available repositories
  * "npackdcl which" to find out which package installed a particular file
  * "npackdcl check" to check dependencies for all installed packages
  * allow short package names in "npackdcl path"
  * allow "npackdcl remove" without specifying a version number if only one version is installed
  * print dependency tree for a package version in "npackdcl info"
  * "npackdcl rm" as an alias for "remove"
  * --debug to show every message
  * add licenses
  * bugfix for the [issue 211](https://code.google.com/p/windows-package-manager/issues/detail?id=211): Remove default repo not work

# 1.17 #

## Npackd ##
  * synchronize information about installed programs with the control panel "Add or remove software" and MSI package database. Allow uninstallation of those packages.
  * 64 bit version. Do not allow the execution of the 32 bit version on 64 bit systems (this does not apply to the command line client)
  * system-wide list of repositories instead of per user
  * gzip compression for all HTTP requests
  * use the Windows Internet cache for repositories, icons
  * show packages in the overview table instead of package versions
  * remove the filters for "not installed" and "newest or installed" packages
  * faster full-text search
  * do not delete desktop shortcuts during installation, fail if a shortcut target cannot be found during package installation
  * only load the necessary package icons, do not download the same icon more than once
  * lower memory usage
  * improve the package table filling performance
  * better error messages while parsing XML, unpacking ZIP files or if a job fails
  * "Feedback"-button
  * better package name validation
  * less borders in the UI
  * change tab order in the settings tab

## Repository format ##
  * no changes

## NpackdCL ##
  * output progress messages in a more simple way
  * search for packages with "npackdcl search -q query"
  * information about a package including all available version numbers: "npackdcl info -p MinGWW64"
  * "npackdcl detect" to synchronize with the MSI package database and the programs installed in the "Add and remove software" control panel
  * check for locked files and directories before uninstalling something
  * output version number

# 1.16 #

## Npackd ##
  * Install, uninstall and update actions support multiple package version selection
  * all long-running operations are performed in the background without blocking the UI
  * Windows 7 taskbar progress visualization
  * plan package updates to minimize side-by-side installations
  * don't use version numbers in package directory names and start menu entries
  * speed improvements: compute SHA1 during the download, download files directly into the package directory, faster update for the NPACKD\_CL variable
  * uninstalling a package will not delete shortcuts from the desktop and Quick Launch anymore
  * show errors as non-modal panels and automatically close them after 30 seconds. This functionality also detects user inactivity.
  * case insensitive sorting for packages
  * show program settings and version information in tabs
  * remove the "Expert" menu
  * better keyboard navigation support
  * much better validation for repositories
  * use 64-bit cmd.exe on 64-bit systems for installation and uninstallation scripts
  * Unicode support for the output of installation and uninstallation scripts
  * format all available WinInet errors with the corresponding message
  * smaller executable
  * choose main menu accelerators automatically
  * bugfix: show the full log if an installation fails
  * bugfix: password protected downloads failed sometimes
  * delete created shortcuts if an installation is cancelled

## Repository format ##
  * "variable" tag to set dependency paths in environment variables
  * UTF-8 for "file" entries

## NpackdCL ##
  * New commands: list, info, update, add-repo, remove-repo
  * path without --versions shows the path of the newest installed version
  * add without --version installs the newest available version
  * Unicode support
  * username/password entry support
  * do not set error codes if a package is already installed or un-installed
  * smaller executable
