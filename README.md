[![Build artifacts](https://ci.appveyor.com/api/projects/status/github/npackd/npackd)](https://ci.appveyor.com/project/npackd/npackd)

# Npackd

* Home page: https://npackd.org
* Downloads: https://github.com/npackd/npackd-cpp/releases
* Forum: https://groups.google.com/forum/#!forum/npackd
* GUI and command line source code: https://github.com/npackd/npackd-cpp
* Web based repository manager source code: https://github.com/npackd/npackd-gae-web

Npackd (pronounced "unpacked") is an application store/package manager/marketplace for applications for Windows. It helps you to find and install software, keep your system up-to-date and uninstall it if no longer necessary. You can watch [this short video](https://www.youtube.com/watch?v=ZLJ8sv6siKQ) to better understand how it works. The process of installing and uninstalling applications is completely automated (silent or unattended installation and un-installation). 

## Add your packages
You can add you packages (a Google account is required) [here](https://npackd.org/package/new).

## Third party tools working with Npackd:
  * [npackd-repoeditor](http://krason.me/software/repoeditor.html) - Editor of small and medium size repositories of Npackd package manager.
  * [Puppet package provider for Npackd](http://forge.puppetlabs.com/badgerious/npackd)
  * [Npackd for Sublime Text Syntax - completions and snippets for creating Npackd XML files](https://sublime.wbond.net/packages/Npackd)
  * [GNU Make](https://github.com/npackd/npackd/wiki/UseInMake)
  * [Npackd plugin for chooie](https://github.com/TomPeters/chooie.Npackd)

## Distribute your applications using Npackd!
You can also distribute your own applications using Npackd: either through your own repository or through the one mentioned above. All you have to do is to package your application as a ZIP file so it is accessible through HTTP and describe it as shown in RepositoryFormat. It is even easier if you already have an .msi or .exe installer. In most cases they can be reused without re-packaging.

Windows is a registered trademark of Microsoft Corporation in the United States and other countries.

