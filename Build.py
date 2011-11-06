# * Qt SDK 1.1.1 must be installed in C:\QtSDK-1.1.1
# * NpackdCL 1.15.8 or later must be installed
# * MinGW does not provide msi.h (Microsoft Installer interface) and the
#     corresponding library. msi.h was taken from the MinGW-W64 project and
#     is committed together with libmsi.a in the Mercurial repository.
#     To re-create the libmsi.a file do the following:
#     * download mingw-w32-bin_i686-mingw_20100914_sezero.zip
#     * run gendef C:\Windows\SysWOW64\msi.dll
#     * run dlltool -D C:\Windows\SysWOW64\msi.dll -V -l libmsi.a

import os
import subprocess
import shutil
import sys
import zipfile
import xml.dom.minidom

class BuildError(Exception):
    '''A build failed'''

    def __init__(self, message):
        self.message = message

    def __str__(self):
        return repr(self.message)

def rmtree_safe(path):
    '''Uses shutil.rmtree, but does not fail if path does not exist'''
    if os.path.exists(path):
        shutil.rmtree(path)

def remove_safe(path):
    '''Uses os.remove, but does not fail if path does not exist'''
    if os.path.exists(path):
        os.remove(path)

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
    def __init__(self):
        self._qtsdk = "C:\\QtSDK-1.1.1"
        self._npackdcl = NpackdCLTool()

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

    def _build_clu(self):
        if not os.path.exists("clu-build-desktop"):
            os.mkdir("clu-build-desktop")

        e = dict(os.environ)
        e["PATH"] = self._qtsdk + "\\mingw\\bin"
        p = subprocess.Popen("\"" + self._qtsdk +
                "\\Desktop\\Qt\\4.7.3\\mingw\\bin\\qmake.exe\" " +
                "..\\clu\\clu.pro " +
                "-r -spec win32-g++ " +
                "CONFIG+=release",
                cwd="clu-build-desktop", env=e)
        if p.wait() != 0:
            raise BuildError("qmake for clu failed")

        p = subprocess.Popen("\"" + self._qtsdk +
                "\\mingw\\bin\\mingw32-make.exe\" ",
                cwd="clu-build-desktop", env=e)
        if p.wait() != 0:
            raise BuildError("make for clu failed")

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

    def _build_qt(self):
        if not os.path.exists("Qt"):
            p = self._npackdcl.path("com.nokia.QtSource", "[4.7.3,4.7.3]")

            shutil.copytree(p, "Qt")
            
        if not os.path.exists("Qt\\bin\\A.exe"):
            f = open('Qt\\yes.txt', 'w')
            f.write('yes')
            f.close()
        
            e = dict(os.environ)
            e["PATH"] = (self._qtsdk + "\\mingw\\bin;" + 
                    self._qtsdk + "Desktop\\Qt\\4.7.3\\mingw\\bin")
                    
            f = open('Qt\\yes.txt', 'r')
            p = subprocess.Popen("Qt\\configure.exe -debug-and-release -platform win32-g++ -opensource -static -no-opengl",
                    stdin=f,
                    cwd="Qt", env=e)
            f.close()
            if p.wait() != 0:
                raise BuildError("configure for Qt failed")

            p = subprocess.Popen("\"" + self._qtsdk +
                    "\\mingw\\bin\\mingw32-make.exe\"",
                    cwd="Qt", env=e)
            if p.wait() != 0:
                raise BuildError("make for Qt failed")

    def _build_zip(self):
        z = zipfile.ZipFile("Npackd.zip", "w", zipfile.ZIP_DEFLATED)
        z.write(self._qtsdk + "\\mingw\\bin\\libgcc_s_dw2-1.dll",
                "libgcc_s_dw2-1.dll")
        z.write(self._qtsdk + "\\mingw\\bin\\mingwm10.dll",
                "mingwm10.dll")
        z.write("wpmcpp\\LICENSE.txt", "LICENSE.txt")
        z.write("wpmcpp\\CrystalIcons_LICENSE.txt", "CrystalIcons_LICENSE.txt")
        z.write("wpmcpp-build-desktop\\release\\wpmcpp.exe", "npackdg.exe")
        for f in ["QtCore4.dll", "QtGui4.dll", "QtNetwork4.dll", "QtXml4.dll"]:
            z.write(self._qtsdk + "\\Desktop\\Qt\\4.7.3\\mingw\\bin\\" + f, f)
        z.write("QuaZIP\\quazip\\release\\quazip.dll", "quazip.dll")
        z.write("zlib\\zlib1.dll", "zlib1.dll")
        z.close()

    def _build_npackdcl_zip(self):
        z = zipfile.ZipFile("NpackdCL.zip", "w", zipfile.ZIP_DEFLATED)
        z.write(self._qtsdk + "\\mingw\\bin\\libgcc_s_dw2-1.dll",
                "libgcc_s_dw2-1.dll")
        z.write(self._qtsdk + "\\mingw\\bin\\mingwm10.dll",
                "mingwm10.dll")
        z.write("wpmcpp\\LICENSE.txt", "LICENSE.txt")
        z.write("npackdcl-build-desktop\\release\\npackdcl.exe", "npackdcl.exe")
        for f in ["QtCore4.dll", "QtNetwork4.dll", "QtXml4.dll"]:
            z.write(self._qtsdk + "\\Desktop\\Qt\\4.7.3\\mingw\\bin\\" + f, f)
        z.write("QuaZIP\\quazip\\release\\quazip.dll", "quazip.dll")
        z.write("zlib\\zlib1.dll", "zlib1.dll")
        z.close()

    def _build_clu_zip(self):
        z = zipfile.ZipFile("CLU.zip", "w", zipfile.ZIP_DEFLATED)
        z.write(self._qtsdk + "\\mingw\\bin\\libgcc_s_dw2-1.dll",
                "libgcc_s_dw2-1.dll")
        z.write(self._qtsdk + "\\mingw\\bin\\mingwm10.dll",
                "mingwm10.dll")
        z.write("wpmcpp\\LICENSE.txt", "LICENSE.txt")
        z.write("clu-build-desktop\\release\\clu.exe", "clu.exe")
        for f in ["QtCore4.dll"]:
            z.write(self._qtsdk + "\\Desktop\\Qt\\4.7.3\\mingw\\bin\\" + f, f)
        z.close()

    def _check_mingw(self):
        if not os.path.exists(self._qtsdk + "\\mingw\\bin\\gcc.exe"):
            raise BuildError('Qt SDK 1.1.1 not found')

    def install_deps(self):
        self._check_mingw()

        if self._npackdcl.location == "":
            raise BuildError("NpackdCL was not found")

        self._npackdcl.add("net.zlib.ZLibSource", "1.2.5")
        self._npackdcl.add("net.sourceforge.quazip.QuaZIPSource", "0.4.2")
        self._npackdcl.add("com.advancedinstaller.AdvancedInstallerFreeware", "8.5")
        self._npackdcl.add("com.selenic.mercurial.Mercurial64", "2")
        # self._npackdcl.add("com.nokia.QtSource", "4.7.3")

    def clean(self):
        rmtree_safe("zlib")
        rmtree_safe("quazip")
        rmtree_safe("wpmcpp-build-desktop")
        rmtree_safe("npackdcl-build-desktop")
        rmtree_safe("clu-build-desktop")
        # rmtree_safe("Qt")
        remove_safe("CLU.zip")
        remove_safe("Npackd.zip")
        remove_safe("NpackdCL.zip")
        remove_safe("npackdg.exe")

    def test(self):
        e = dict(os.environ)
        e["PATH"] = (self._qtsdk + "\\Desktop\\Qt\\4.7.3\\mingw\\bin;" + 
                "QuaZIP\\quazip\\release;zlib")
                
        p = subprocess.Popen("npackdcl-build-desktop\\release\\npackdcl.exe",
                env=e)
        err = p.wait()
        if err != 1:
            raise BuildError("npackdcl without arguments should return 1, but was %d" % err)
            
        p = subprocess.Popen("npackdcl-build-desktop\\release\\npackdcl.exe unit-tests",
                env=e)
        err = p.wait()
        if err != 0:
            raise BuildError("npackdcl unit tests failed")
            
        # add should return 0 for an already installed package
        p = subprocess.Popen("npackdcl-build-desktop\\release\\npackdcl.exe add -p net.zlib.ZLibSource -v 1.2.5",
                env=e)
        p.wait()
        p = subprocess.Popen("npackdcl-build-desktop\\release\\npackdcl.exe add -p net.zlib.ZLibSource -v 1.2.5",
                env=e)
        err = p.wait()
        if err != 0:
            raise BuildError("npackdcl should return 0 for an already installed package, but was %d" % err)
            
        # remove should return 0 for an already un-installed package
        p = subprocess.Popen("npackdcl-build-desktop\\release\\npackdcl.exe remove -p net.zlib.ZLibSource -v 1.2.5",
                env=e)
        p.wait()
        p = subprocess.Popen("npackdcl-build-desktop\\release\\npackdcl.exe remove -p net.zlib.ZLibSource -v 1.2.5",
                env=e)
        err = p.wait()
        if err != 0:
            raise BuildError("npackdcl should return 0 for an already un-installed package, but was %d" % err)
            
        print("All tests passed")

    def push(self):
        '''Push the changes to the default Mercurial repository'''
        xml.dom.minidom.parse("repository\\Rep.xml")
        xml.dom.minidom.parse("repository\\Rep64.xml")
        xml.dom.minidom.parse("repository\\RepUnstable.xml")
        xml.dom.minidom.parse("repository\\Libs.xml")
        
        loc = self._npackdcl.path("com.selenic.mercurial.Mercurial64", 
                "[1.8,3)") + "\\hg.exe"
        p = subprocess.Popen("\"" + loc + "\" push")
        if p.wait() != 0:
            raise BuildError("hg push failed")
        
    def build(self):
        self._project_path = os.path.abspath("")

        self._build_zlib()
        self._build_quazip()
        #self._build_qt()

        self._build_wpmcpp()
        self._build_npackdcl()
        self._build_clu()

        self._build_msi()
        self._build_zip()
        self._build_npackdcl_zip()
        self._build_clu_zip()

    def help(self):
        print("Build.py help to show this help")
        print("Build.py build to build everything")
        print("Build.py clean to clean generated files")
        print("Build.py test to run internal Npackd tests")
        print("Build.py push to push the changes to the Npackd Mercurial repository")

build = Build()
try:
    build.install_deps()
    if len(sys.argv) == 2 and sys.argv[1] == "clean":
        build.clean()
    elif len(sys.argv) == 2 and sys.argv[1] == "push":
        build.push()
    elif len(sys.argv) == 2 and sys.argv[1] == "test":
        build.test()
    elif len(sys.argv) == 2 and sys.argv[1] == "build":
        build.build()
    else:
        build.help()
except BuildError as e:
    print('Build failed: ' + e.message)
