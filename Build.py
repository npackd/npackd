import os
import subprocess
import shutil
import sys

class BuildError(Exception):
    '''A build failed'''
    
    def __init__(self, message):
        self.message = message
        
    def __str__(self):
        return repr(self.message)

def rm_existing_tree(path):
    '''Uses shutil.rmtree, but does not fail if path does not exist'''
    if os.path.exists(path):
        shutil.rmtree(path)

def capture_process_output_line(cmd):
    '''Captures the output of a process.'''
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output, errors = p.communicate()
    return output.decode("utf-8", "strict").strip()
    
def needs_update(source, dest):
    '''Returns True if the dest file have to be re-created'''
    res = True
    if os.path.exists(source):
        res = not os.path.exists(dest) or (
                os.path.getmtime(source) >= os.path.getmtime(dest))
    else:
        res = not os.path.exists(dest)
    return res

class NpackdCLTool:
    '''NpackdCL'''
    
    def __init__(self):
        p = os.environ['NPACKD_CL']
        cl = p + '\\npackdcl.exe'
        if p.strip() == '' or not os.path.exists(cl):
            self.location = ""
        else:
            self.location = cl
        
    def add(self, package, version):
        '''Installs a package
        
        package package name like "com.test.Editor"
        version version number like "1.2.3"
        '''
        prg = "\"" + self.location + "\""
        p = subprocess.Popen(prg + " " + 
                " add --package=" + package + " --version=" + version)
        err = p.wait()
        
        # NpackdCL returns 1 if a package is already installed
        #if err != 0:
        #    raise BuildError("Installation of %s %s via NpackdCL failed. Returned error code: %d" % (package, version, err))

    def path(self, package, versions):
        '''Finds an installed package
        
        package package name like "com.test.Editor"
        versions version range like "[2, 3.1)"
        '''
        return capture_process_output_line("\"" + self.location + "\" " +
                " path --package=" + package + " --versions=" + versions)
        
class Build:
    '''* Qt SDK 1.1.1
    * MinGW does not provide msi.h (Microsoft Installer interface) and the 
    corresponding library. msi.h was taken from the MinGW-W64 project and 
    is committed together with libmsi.a in the Mercurial repository. 
    To re-create the libmsi.a file do the following:
    * download mingw-w32-bin_i686-mingw_20100914_sezero.zip
    * run gendef C:\Windows\SysWOW64\msi.dll 
    * run dlltool -D C:\Windows\SysWOW64\msi.dll -V -l libmsi.a
    '''

    def __init__(self):
        self._qtsdk = "C:\\QtSDK-1.1.1"
        
    def _build_wpmcpp(self):
        if not os.path.exists("wpmcpp-build-desktop"):
            os.mkdir("wpmcpp-build-desktop")
            
        e = dict(os.environ)
        e["PATH"] = self._qtsdk + "\\mingw\\bin"
        p = subprocess.Popen("\"" + self._qtsdk + 
                "\\Desktop\\Qt\\4.7.3\\mingw\\bin\\qmake.exe\" " + 
                "..\\wpmcpp\\wpmcpp.pro " +
                "-r -spec win32-g++ " +
                "CONFIG+=release",
                cwd="wpmcpp-build-desktop", env=e)
        if p.wait() != 0:
            raise BuildError("qmake for wpmcpp failed")
            
        p = subprocess.Popen("\"" + self._qtsdk + 
                "\\mingw\\bin\\mingw32-make.exe\" ", 
                cwd="wpmcpp-build-desktop", env=e)
        if p.wait() != 0:
            raise BuildError("make for wpmcpp failed")

    def _build_msi(self):
        loc = self._npackdcl.path("com.advancedinstaller.AdvancedInstallerFreeware", "[8.4,9)")
        p = subprocess.Popen(
                "\"" + loc + "\\bin\\x86\\AdvancedInstaller.com\" " + 
                "/build wpmcpp.aip", 
                cwd="wpmcpp")
        if p.wait() != 0:
            raise BuildError("msi creation failed")

    def _build_npackdcl(self):
        if not os.path.exists("npackdcl-build-desktop"):
            os.mkdir("npackdcl-build-desktop")
            
        e = dict(os.environ)
        e["PATH"] = self._qtsdk + "\\mingw\\bin"
        p = subprocess.Popen("\"" + self._qtsdk + 
                "\\Desktop\\Qt\\4.7.3\\mingw\\bin\\qmake.exe\" " + 
                "..\\npackdcl\\npackdcl.pro " +
                "-r -spec win32-g++ " +
                "CONFIG+=release",
                cwd="npackdcl-build-desktop", env=e)
        if p.wait() != 0:
            raise BuildError("qmake for npackdcl failed")

        p = subprocess.Popen("\"" + self._qtsdk + 
                "\\mingw\\bin\\mingw32-make.exe\" ", 
                cwd="npackdcl-build-desktop", env=e)
        if p.wait() != 0:
            raise BuildError("make for npackdcl failed")

    def _build_zlib(self):
        if not os.path.exists("zlib"):
            p = self._npackdcl.path("net.zlib.ZLibSource", "[1.2.5,1.2.5]")
            shutil.copytree(p, "zlib")
            e = dict(os.environ)
            e["PATH"] = self._qtsdk + "\\mingw\\bin"
            p = subprocess.Popen("\"" + self._qtsdk + 
                    "\\mingw\\bin\\mingw32-make.exe\" -f win32\Makefile.gcc", 
                    cwd="zlib", env=e)
            if p.wait() != 0:
                raise BuildError("make for zlib failed")

    def _build_quazip(self):
        if not os.path.exists("QuaZIP"):
            p = self._npackdcl.path("net.sourceforge.quazip.QuaZIPSource", "[0.4.2,0.4.2]")

            # QuaZIP searches for -lz.dll...
            shutil.copy("zlib\\libzdll.a", "zlib\\libz.dll.a")
            
            shutil.copytree(p, "QuaZIP")
            e = dict(os.environ)
            e["PATH"] = self._qtsdk + "\\mingw\\bin"
            p = subprocess.Popen("\"" + self._qtsdk + 
                    "\\Desktop\\Qt\\4.7.3\\mingw\\bin\\qmake.exe\" " + 
                    "CONFIG+=release "
                    "INCLUDEPATH=" + self._project_path + "\\zlib " + 
                    "LIBS+=-L" + self._project_path + "\\zlib " + 
                    "LIBS+=-L" + self._project_path + "\\QuaZIP\\quazip\\release", 
                    cwd="QuaZIP", env=e)
            if p.wait() != 0:
                raise BuildError("qmake for quazip failed")
            
            p = subprocess.Popen("\"" + self._qtsdk + 
                    "\\mingw\\bin\\mingw32-make.exe\"", 
                    cwd="QuaZIP", env=e)
            if p.wait() != 0:
                raise BuildError("make for quazip failed")
                
    def _build_zip(self):
        sz = self._npackdcl.path("org.7-zip.SevenZIP", 
                "[9.20,9.20]") + "\\7z.exe"
        d = os.getcwd()
            
        zip_file = d + "\\Npackd.zip"
        if os.path.exists(zip_file):
            os.remove(zip_file)
        
        start = "\"" + sz + "\" " + "a " + "\"" + zip_file + "\" "
        
        p = subprocess.Popen(start +
                "CrystalIcons_LICENSE.txt " + 
                "LICENSE.txt", cwd="wpmcpp")
        if p.wait() != 0:
            raise BuildError("ZIP creation failed")
                
        p = subprocess.Popen(start + 
                "libgcc_s_dw2-1.dll " + 
                "mingwm10.dll", cwd=self._qtsdk + "\\mingw\\bin\\")
        if p.wait() != 0:
            raise BuildError("ZIP creation failed")

        p = subprocess.Popen(start + 
                "QtCore4.dll " + 
                "QtGui4.dll " + 
                "QtNetwork4.dll " + 
                "QtXml4.dll", 
                cwd=self._qtsdk + "\\Desktop\\Qt\\4.7.3\\mingw\\bin\\")
        if p.wait() != 0:
            raise BuildError("ZIP creation failed")

        shutil.copyfile("wpmcpp-build-desktop\\release\\wpmcpp.exe",
                "npackdg.exe")
                
        p = subprocess.Popen(start + 
                "npackdg.exe")
        if p.wait() != 0:
            raise BuildError("ZIP creation failed")

        p = subprocess.Popen(start + 
                "quazip.dll", 
                cwd="QuaZIP\\quazip\\release\\")
        if p.wait() != 0:
            raise BuildError("ZIP creation failed")

        p = subprocess.Popen(start + 
                "zlib1.dll", cwd="zlib\\")
        if p.wait() != 0:
            raise BuildError("ZIP creation failed")

    def _check_mingw(self):
        if not os.path.exists(self._qtsdk + "\\mingw\\bin\\gcc.exe"):
            raise BuildError('Qt SDK 1.1.1 not found')
       
    def clean(self):
        rm_existing_tree("zlib")
        rm_existing_tree("quazip")
        rm_existing_tree("wpmcpp-build-desktop")
        rm_existing_tree("npackdcl-build-desktop")

    def build(self):
        try:
            self._project_path = os.path.abspath("")
            
            self._check_mingw()

            self._npackdcl = NpackdCLTool()
            if self._npackdcl.location == "":
                raise BuildError("NpackdCL was not found") 
            
            self._npackdcl.add("net.zlib.ZLibSource", "1.2.5")
            self._npackdcl.add("net.sourceforge.quazip.QuaZIPSource", "0.4.2")
            self._npackdcl.add("com.advancedinstaller.AdvancedInstallerFreeware", "8.4")
            self._npackdcl.add("org.7-zip.SevenZIP", "9.20")
                
            self._build_zlib()            
            self._build_quazip()            
            self._build_wpmcpp()            
            self._build_npackdcl()            
            self._build_msi()            
            self._build_zip()
        except BuildError as e:
            print('Build failed: ' + e.message)

build = Build()            
if len(sys.argv) == 2 and sys.argv[1] == "clean": 
    build.clean()
else: 
    build.build()
