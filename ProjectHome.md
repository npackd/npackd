# Npackd #

## Home page: https://npackd.appspot.com ##

Npackd (pronounced "unpacked") is an application store/package manager/marketplace for applications for Windows. It helps you to find and install software, keep your system up-to-date and uninstall it if no longer necessary. You can watch [this short video](http://www.youtube.com/watch?v=7ZkJ2i2xbow) to better understand how it works. The process of installing and uninstalling applications is completely automated (silent or unattended installation and un-installation). There is also a [command line](CommandLine.md) based version of Npackd which you can [install](CommandLineInstallation.md) from the command line:

```
C:\> msiexec.exe /qb- /i http://bit.ly/npackdcl-1_19_13
```

see [What is new in Npackd](ChangeLog.md)

## Project status ##
[![](https://ci.appveyor.com/api/projects/status/f0sqwvjci85dx5q0?notused=image.png)](https://ci.appveyor.com/project/tim-lebedkov/windows-package-manager-npackd-cpp/build/artifacts)

## Add your packages ##
You can add you packages (a Google account is required) [here](https://npackd.appspot.com/package/new).

## Suggest software ##
You can [suggest](http://code.google.com/p/windows-package-manager/issues/entry?template=Suggest%20software) a package for inclusion if it is not yet available.

## News ##
You can follow the news on [Twitter](http://twitter.com/Npackd)

<a href='Hidden comment: 
Alternative Twitter RSS: http://www.twitter-rss.com/user_timeline.php?screen_name=npackd

<wiki:gadget url="http://google-code-feed-gadget.googlecode.com/svn/trunk/gadget.xml" up_feeds="http://www.twitter-rss.com/user_timeline.php?screen_name=npackd" width="500" height="400" border="0"/>
'></a>

## RSS ##
  * [Repository changes](http://code.google.com/feeds/p/windows-package-manager/hgchanges/basic)
  * [Wiki changes](http://code.google.com/feeds/p/windows-package-manager/hgchanges/basic?repo=wiki)
  * [Npackd C++ source changes](http://code.google.com/feeds/p/windows-package-manager/hgchanges/basic?repo=npackd-cpp)
  * [npackd.appspot.com source changes](http://code.google.com/feeds/p/windows-package-manager/hgchanges/basic?repo=npackd-gae-web)
  * [Issue updates](http://code.google.com/feeds/p/windows-package-manager/issueupdates/basic)

## List of Google+ posts about Npackd ##
https://www.google.com/search?q=site%3Aplus.google.com+npackd

## Third party tools working with Npackd: ##
  * [Npackd plugin for chooie](https://github.com/TomPeters/chooie.Npackd)
  * [Puppet package provider for Npackd](http://forge.puppetlabs.com/badgerious/npackd)
  * [Npackd for Sublime Text Syntax - completions and snippets for creating Npackd XML files](https://sublime.wbond.net/packages/Npackd)

## Distribute your applications using Npackd! ##
You can also distribute your own applications using Npackd: either through your own repository or through the one mentioned above. All you have to do is to package your application as a ZIP file so it is accessible through HTTP and describe it as shown in RepositoryFormat. It is even easier if you already have an .msi or .exe installer. In most cases they can be reused without re-packaging.

## Default repository ##
The default repository is located at `https://npackd.appspot.com/rep/xml?tag=stable` and currently contains [hundreds of applications](https://npackd.appspot.com/rep/xml?tag=stable). [Read more...](DefaultRepository.md)

## Default repository/64 bit ##
The default repository for 64-bit software is located at `https://npackd.appspot.com/rep/xml?tag=stable64` and contains [only 64-bit versions](https://npackd.appspot.com/rep/xml?tag=stable64) of some packages from the stable repository. [Read more...](DefaultRepository64Bit.md)

## Default programming libraries repository ##
The default programming libraries repository is located at `https://npackd.appspot.com/rep/xml?tag=libs` and contains [only programming libraries](https://npackd.appspot.com/rep/xml?tag=libs) (mostly source code). [Read more...](LibrariesRepository.md)

## Default repository for unstable software ##
The default repository for unstable software is located at `https://npackd.appspot.com/rep/xml?tag=unstable` and contains [only unstable versions](https://npackd.appspot.com/rep/xml?tag=unstable) of some packages from the stable repository. [Read more...](DefaultRepositoryUnstableSofware.md)

## Default repository for Vim plugins ##
The default repository for Vim plugins is located at `http://npackd.appspot.com/rep/xml?tag=vim` and contains [only plugins for Vim](http://npackd.appspot.com/rep/xml?tag=vim). [Read more...](DefaultRepositoryVimPlugins.md)

## vim.org repository for Vim plugins ##
The vim.org repository for Vim plugins is regularly (currently every 3 months) exported to an Npackd repository located at `http://downloads.sourceforge.net/project/npackd/VimOrgRep.xml`. Currently only plugins in .zip and .tar.gz files without additional directory levels are supported, the repository contains over 1400 packages.

## 3rd party repositories ##
[C++ libraries for Microsoft Visual Studio 2010 64-bit](http://www.maths.lth.se/matematiklth/personal/petter/packages/)

## Main features ##
  * synchronizes information about installed programs with the control panel "Add or remove software" and MSI package database. Allow uninstallation of those packages.
  * support for proxies (use the internet settings control panel to configure it)
  * password protected pages. This can be used to restrict access to your repository.
  * fast installation and uninstallation without user interaction. A typical application is installed and uninstalled in seconds (downloading the package is the most lengthy operation)
  * dependencies
  * shortcuts in the start menu are automatically created/deleted
  * multiple program versions can be installed side-by-side
  * cryptographic checksum for packages (SHA1)
  * prevents uninstallation of running programs

&lt;wiki:gadget url="https://www.ohloh.net/p/windows-package-manager/widgets/project\_basic\_stats.xml" height="230" width="350" border="0" /&gt;
<a href='http://www.addthis.com/bookmark.php?v=250&amp;username=xa-4c376eea7c4cc880'><img src='https://s7.addthis.com/static/btn/v2/lg-share-en.gif' alt='Bookmark and Share' width='125' height='16' /></a>


Windows is a registered trademark of Microsoft Corporation in the United States and other countries.