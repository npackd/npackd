The command line based installation could be useful for scripting automatic installations for new machines or VMs.

[NpackdCL](CommandLine.md) package is also available as .msi. msiexec.exe can be used to download the file and install it using the following command:

```
C:\> msiexec.exe /qb- /norestart /i http://bit.ly/npackdcl-1_16_4 
```

Please note that http://bit.ly/npackdcl-1_16_4 is just a shorter URL for http://windows-package-manager.googlecode.com/files/NpackdCL-1.16.4.msi

You can set the APPDIR property to install the package in another directory rather than in C:\Program Files (x86). Changing the PATH variable would allow to use npackdcl.exe directly from the same shell:
```
C:\> msiexec.exe /qb- /norestart /i http://bit.ly/npackdcl-1_16_4 APPDIR=C:\NpackdCL && SET PATH=C:\NpackdCL;%PATH%
```


Here is the whole list of command line arguments for msiexec.exe: http://technet.microsoft.com/en-us/library/cc759262%28v=ws.10%29.aspx