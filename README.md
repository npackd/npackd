== Project status ==
[https://ci.appveyor.com/project/tim-lebedkov/windows-package-manager-npackd-cpp/build/artifacts https://ci.appveyor.com/api/projects/status/f0sqwvjci85dx5q0?notused=image.png]

== Add your packages ==
You can add you packages (a Google account is required) [https://npackd.appspot.com/package/new here].

== Suggest software ==
You can [https://github.com/tim-lebedkov/npackd/issues/new suggest] a package for inclusion if it is not yet available.

== News ==
You can follow the news on [http://twitter.com/Npackd Twitter]

<wiki:comment>
Alternative Twitter RSS: http://www.twitter-rss.com/user_timeline.php?screen_name=npackd

<wiki:gadget url="http://google-code-feed-gadget.googlecode.com/svn/trunk/gadget.xml" up_feeds="http://www.twitter-rss.com/user_timeline.php?screen_name=npackd" width="500" height="400" border="0"/>
</wiki:comment>

== Third party tools working with Npackd: ==
  * [https://github.com/TomPeters/chooie.Npackd Npackd plugin for chooie]
  * [http://forge.puppetlabs.com/badgerious/npackd Puppet package provider for Npackd] 
  * [https://sublime.wbond.net/packages/Npackd Npackd for Sublime Text Syntax - completions and snippets for creating Npackd XML files]

==Distribute your applications using Npackd!==
You can also distribute your own applications using Npackd: either through your own repository or through the one mentioned above. All you have to do is to package your application as a ZIP file so it is accessible through HTTP and describe it as shown in RepositoryFormat. It is even easier if you already have an .msi or .exe installer. In most cases they can be reused without re-packaging.

==Default repository==
The default repository is located at `https://npackd.appspot.com/rep/xml?tag=stable` and currently contains [https://npackd.appspot.com/rep/xml?tag=stable hundreds of applications]. [DefaultRepository Read more...]

==Default repository/64 bit==
The default repository for 64-bit software is located at `https://npackd.appspot.com/rep/xml?tag=stable64` and contains [https://npackd.appspot.com/rep/xml?tag=stable64 only 64-bit versions] of some packages from the stable repository. [DefaultRepository64Bit Read more...]

==Default programming libraries repository==
The default programming libraries repository is located at `https://npackd.appspot.com/rep/xml?tag=libs` and contains [https://npackd.appspot.com/rep/xml?tag=libs only programming libraries] (mostly source code). [LibrariesRepository Read more...]

==Default repository for unstable software==
The default repository for unstable software is located at `https://npackd.appspot.com/rep/xml?tag=unstable` and contains [https://npackd.appspot.com/rep/xml?tag=unstable only unstable versions] of some packages from the stable repository. [DefaultRepositoryUnstableSofware Read more...]

==Default repository for Vim plugins==
The default repository for Vim plugins is located at `http://npackd.appspot.com/rep/xml?tag=vim` and contains [http://npackd.appspot.com/rep/xml?tag=vim only plugins for Vim]. [DefaultRepositoryVimPlugins Read more...]

==vim.org repository for Vim plugins==
The vim.org repository for Vim plugins is regularly (currently every 3 months) exported to an Npackd repository located at `http://downloads.sourceforge.net/project/npackd/VimOrgRep.xml`. Currently only plugins in .zip and .tar.gz files without additional directory levels are supported, the repository contains over 1400 packages.

==3rd party repositories==
[http://www.maths.lth.se/matematiklth/personal/petter/packages/ C++ libraries for Microsoft Visual Studio 2010 64-bit]

==Main features==
  * synchronizes information about installed programs with the control panel "Add or remove software" and MSI package database. Allow uninstallation of those packages. 
  * support for proxies (use the internet settings control panel to configure it)
  * password protected pages. This can be used to restrict access to your repository.
  * fast installation and uninstallation without user interaction. A typical application is installed and uninstalled in seconds (downloading the package is the most lengthy operation)
  * dependencies
  * shortcuts in the start menu are automatically created/deleted
  * multiple program versions can be installed side-by-side
  * cryptographic checksum for packages (SHA1)
  * prevents uninstallation of running programs

<wiki:gadget url="https://www.ohloh.net/p/windows-package-manager/widgets/project_basic_stats.xml" height="230" width="350" border="0" />
<a class="addthis_button" href="http://www.addthis.com/bookmark.php?v=250&amp;username=xa-4c376eea7c4cc880"><img src="https://s7.addthis.com/static/btn/v2/lg-share-en.gif" width="125" height="16" alt="Bookmark and Share" style="border:0"/></a>


Windows is a registered trademark of Microsoft Corporation in the United States and other countries.
