Change the version number in wpmcpp\version.txt

## Translations ##
  * update translations

## Tests ##
  * unit tests in npackdcl.exe
  * wpmcpp\TestCases.txt
  * npackdcl\TestCases.txt
  * wpmcpp\RunDrMemory.bat: install and remove AbiWord

## Versioning ##
Tag the version in Mercurial, commit, push.

## Packages ##
Upload to code.google.com/p/windows-package-manager:
  * wpmcpp\32\Npackd32-X.XX.X.msi
  * wpmcpp\32\Npackd32-X.XX.X.zip
  * wpmcpp\32\Npackd32-X.XX.X.map
  * wpmcpp\64\Npackd64-X.XX.X.msi
  * wpmcpp\64\Npackd64-X.XX.X.zip
  * wpmcpp\64\Npackd64-X.XX.X.map
  * npackdcl\32\NpackdCL-X.XX.X.msi
  * npackdcl\32\NpackdCL-X.XX.X.zip
  * npackdcl\32\NpackdCL-X.XX.X.map

Create packages on npackd.appspot.com in the **unstable** repository for:
  * Npackd 32 bit
  * Npackd 64 bit
  * NpackdCL

## Dev packages ##
Create the following packages from the repository
  * hg clone -u NpackdVersion\_X.XX.X https://tim.lebedkov@code.google.com/p/windows-package-manager.npackd-cpp/ X.XX.X\_32
  * hg clone -u NpackdVersion\_X.XX.X https://tim.lebedkov@code.google.com/p/windows-package-manager.npackd-cpp/ X.XX.X\_64

Execute the following scripts in X.XX.X\_32:
  * build projects for 32 bit

Execute the following scripts in X.XX.X\_64:
  * build projects for 64 bit

Create packages on npackd.appspot.com in the **libs** repository for:
  * Npackd dev x86\_64 w64
  * Npackd dev i686 w64

## Issues ##
Mark all issues with Milestone-X.XX as FixedPleaseTest

## bit.ly ##
Create a bit.ly link, e.g. http://bit.ly/npackdcl-1_17_9

## Announcement ##
Announce the availability of the new alpha version on Twitter and on the Forum.

## Update the Wiki ##
Create/update the "ChangeLog" and "CommandLine" pages in the Wiki.

## Beta ##
If no significant bugs are found during 2 weeks, promote the version to a beta.

## Final ##
If no significant bugs are found during 4 weeks after the beta release, promote the version to a stable and move the packages to the stable repository.
  * update the NpackdCL installation command on the home page
  * update the link to "What is new" on the home page
  * update the screenshot on the home page http://npackd.appspot.com
  * change the downloads page
  * upload the new screenshot to sf.net/projects/npackd
  * mark the Npackd 32/64 bit and NpackdCL packages on npackd.appspot.com as "stable"
  * change the stable version on sf.net
  * post to Twitter and to the forum
  * post a "What is new in X.XX" article on dzone.com
  * upload a screencast with the overview of new features to YouTube
    * replace the screencast on code.google.com/p/windows-package-manager
    * post the link on Twitter
    * post the link on the forum
    * post the link on dzone.com
