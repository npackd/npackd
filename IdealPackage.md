# Introduction #

An ideal package should meet all the expectations a user would have from
a software installed on his system. These expectations are: the whole system
should remain fast, secure, predictable and reproducible after the installation
of a package. Predictable in this context means that all the changes to the
system were expected by the user. Reproducible means that the same packages
installed on another computer would yield the same results. The installed
package should
either fulfill a specific need of the user or make the whole system faster,
or more secure. Sometimes a package would improve one aspect
and worsen the other. For example, installation of a anti virus software
makes a computer more secure, but also slower. Such a trade-off and any other
deviations from the rules below should be
clearly indicated to the user via the package description. The rest of this
document applies these basic goals to real packages and shows best practices and
common pitfalls.

# Download location #

Package installation starts with the download. A download location should be
fast. It should be fast for all your users. If your software
can be downloaded around the world, your download location should be fast
regardless of the physical user location. This means in most cases that a
content delivery system should be used. The main download URL could for example
find out the geographical location of the user via the IP address and redirect
to another URL that is closer to the user.

A package download should also be secure. This means that HTTPS should be used.
The Npackd repository format allows the definition of an SHA1
sum for the downloaded binary. This possibility should also be used to prevent
binary change for a hacked download location. Sometimes software vendors publish
a newer program version at the same location. In this case SHA1 definition in
the repository cannot be used making the download less secure.

It is also very important to have a reproducible download location. Once a binary
is referenced, it should never be deleted or moved to a new location, even if
there is a newer version of the program available. The corresponding repository
may not yet be updated and the installation of the previous package version
would fail. Often users prefer an older version of a program for some
reason. It may be easier to use, does not have a particular bug etc. Especially
for software development it is critical to use a particular version of a
package instead of the newest available. All this also applies even to small
fixes. The version number should
be increased and the new version should be published at a new URL.

The discovery of new package versions could be made easier or even automated
with a little help from the software vendor. The package site should clearly
state which version is currently considered stable.
Example: "Current stable version: 17.2"

# Versioning #

Npackd only supports numeric version numbers. If possible, packages should
follow this scheme. If code names are used for particular package versions, it
should be clear which real package versions should be used instead.

# Package format #

Npackd supports packages in ZIP format natively. All other installation formats
are unpacked and installed using specific tools. ZIP based packages are faster
to install
and remove and are therefore preferred. For example, MSI based packages keep
the list of the files installed on the system
and only delete the installed files during the package removal. Npackd on the
contrary always deletes the whole directory where a package is installed, which
is much faster. Distributing your application as a ZIP
file does not mean that a complicated installation procedure is not possible.
The usage of the Windows command shell or other programs via dependencies is
still possible. If you also would like to have an installer that works without
Npackd, we recommend using InnoSetup, NSIS or MSI based installers which are
known to work well with Npackd.

There is no difference in security as all packages are installed using the
administrative privileges. Current version of Npackd (1.17.9) also does not
handle signed binaries in any special way. Starting with Windows Vista Microsoft
only allows installation of signed drivers.

It is important, that the package installer supports silent installation without
showing any confirmation or progress dialogs. The same applies to the package
removal. Additionally the installer should support installation in a predefined
existing directory.

So called light installers that download the rest of the package during the
installation are also undesirable. The installer could use another set of
proxies than Npackd or other network protocols that could be blocked.

# Dependencies #

A package vendor should clearly describe all dependencies including the valid
range of versions and whether they are optional. The installation and removal
scripts should only expect those software packages to be installed that were
mentioned as a dependency. The installation location of a dependency should not
be assumed to be in %ProgramFiles%, npackdcl.exe should be used instead. Since
repository format 3 it is also possible to specify an environment variable
that will be set automatically (see RepositoryFormat for more information).

# Changes on the computer #

A package installation should not make the system significantly slower.

The very same rule applies to security: a package should not make the system
less secure.

During installation a package changes different parts of the operating system we
would call "resources" in the following. The first resource every package uses
is the installation directory. Npackd chooses automatically a non-existing name
so no conflicts will arise here. All other changes to the system should also be
done so that they do not conflict with other packages or other versions of the
same package. A package should not change the default settings for a
resource  during installation. Example: the default editor for a file type or
default browser search engine should not be changed by a package installation.

## User-level settings ##

All packages are installed by Npackd on the system level. A package should never
change the settings of the current user only.

## Entries in the start menu ##

Each package version can define different entries for the start menu
(see RepositoryFormat). Npackd manages these entries automatically. They are
created whenever a package is installed and deleted if a package gets
uninstalled. Version number and package title are added to the file names like
"ABC (2.4.1)" or "Read Me (ABC 2.4.1)" if necessary so that there will be no
conflicts between different packages or different versions of the same package.

## Shortcut on the desktop ##

One of the often used changes to the system is the installation of a
shortcut on the desktop. This change is unexpected as all shortcuts are already
in the start menu. Additionally this resource is limited:
only a few icons may be placed on the desktop and still be easily found by the
user.

## Shortcut to the uninstaller in the Start Menu ##

Another often used and unwanted change is the installation of the shortcut
to the uninstaller in the start menu. This is superfluous and unnecessary
because the package removal is managed by Npackd.

## Shortcuts in the Quick Launch Bar ##

The shortcuts in the Quick Launch Bar should not be created by an
installation. The Quick Launch Bar is a limited resource and is user specific.

## PATH ##

Packages should not change the PATH variable during installation or removal.
The same applies to CLASSPATH (Java class path),
PYTHONPATH (Python module path) and similar variables.

The PATH variable cannot be perfectly shared between different packages as the
order of entries is significant. A file on PATH that comes from one package can
be hidden by another file with the same name from another package.

Adding libraries to the PATH
does not make much sense as library packages tend to have many incompatible
versions. Adding libraries to the PATH increases the risk of a wrong library
version being chosen. Some programs find wrong libraries and crash too (e.g. gcc).

The PATH becomes very long very fast. The existing limit in Windows for the length of this
variable is 2048 characters. Although NpackdCL is aware of this limit, not all
installers are. Many installers just clear the PATH making the whole system
almost unusable. Many shortcuts in the start menu stop to work as they assume
that at least C:\Windows and C:\Windows\System32 are in the PATH.  Many programs
just start to crash (e.g. ClickOnce installer).

One of the possible ways to deal with all these limitations is to set PATH
dynamically for each package version.

## Certificates ##

Installation of a certificate lowers the security of the system and should be
clearly indicated.