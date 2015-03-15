# Introduction #

To host your own repository you would need an HTTP server and the repository XML file.

# Default Npackd repository #

Everybody with a Google account can create packages at http://npackd.appspot.com/package/new

# Getting an HTTP server #

There are different ways to get an HTTP server:
  * on many Windows server systems IIS is already installed. All you need is to put your XML file under `C:\InetPub\wwwroot\MyRepository.xml` and access it via http://localhost/MyRepository.xml
  * you could host your public repository on http://github.com as a gist. Example: https://gist.github.com/4132983 Please note that you should access this repository using the "raw" link: https://gist.github.com/raw/4132983/dabecde48c796d4fdfa2f645bb744ac58640572c/TestRepository.xml
  * install Apache via Npackd, start it from the Windows services control panel, put your XML repository under `C:\Program Files (x86)\Apache_HTTP_Server\htdocs\MyRepository.xml` and access it via http://localhost:8083/MyRepository.xml
  * install npackd-gae-web (http://code.google.com/p/windows-package-manager/source/checkout?repo=npackd-gae-web) on GAE (https://developers.google.com/appengine/) and host your packages there. Example: http://npackd.appspot.com

# Cloud based solutions #
## Dropbox ##
For testing purpose or limited sharing, you may use Dropbox. To do so, simply copy your repository XML in a folder and share this file. In this case, you will be using the web server provided by Dropbox.

## Google Drive ##
Upload a file to your Google Drive folder, share it with everybody and use the URL in the form
https://drive.google.com/uc?export=download&id=ID . ID is here the ID of your file. To find the ID of a file press the right mouse button over a file and choose "Share.../Share...". The shown URL will look like this one:
https://docs.google.com/file/d/0Bzt53EW9ybrwTFFUM0NwN2xtaHM/edit?usp=sharing.
0Bzt53EW9ybrwTFFUM0NwN2xtaHM is the ID of the file.
Although https://drive.google.com/uc?export=download&confirm=no_antivirus&id=ID
can be used for large files that cannot be scanned for viruses by Google, a
warning will be shown. This makes it impossible to host binaries over 25 MB on
Google Drive. Additional difficulty is that the URL does not contain the file
name and the files downloaded by Npackd would be always named "uc".

## Microsoft SkyDrive ##
Microsoft SkyDrive unfortunately does not provide stable URLs to the public
files.