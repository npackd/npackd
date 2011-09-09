import os
import subprocess
import shutil

class Build:
    def _build_mingw_utils(self):
        ret = False
        if not os.path.exists("mingw-utils"):
            p = self._capture(self._npackd_cl + ' path --package=org.mingw.MinGWUtilities --versions=[0.3,0.3]')
            if p.strip() == '':
                print('mingw-utils 0.3 was not found')
            else:
                shutil.copytree(p, "mingw-utils")
                ret = True
        else:
            ret = True
            
        return ret

    def _build_zlib(self):
        ret = False
        if not os.path.exists("zlib"):
            p = self._capture(self._npackd_cl + ' path --package=net.zlib.ZLibSource --versions=[1.2.5,1.2.5]')
            if p.strip() == '':
                print('zlib 1.2.5 was not found')
            else:
                shutil.copytree(p, "zlib")
                e = dict(os.environ)
                e["PATH"] = self._qtsdk + "\\mingw\\bin"
                p = subprocess.Popen("\"" + self._qtsdk + 
                        "\\mingw\\bin\\mingw32-make.exe\" -f win32\Makefile.gcc", 
                        cwd="zlib", env=e)
                p.wait()               
                ret = True
        else:
            ret = True
            
        return ret

    def _build_quazip(self):
        ret = False
        if not os.path.exists("QuaZIP"):
            p = self._capture(self._npackd_cl + ' path --package=net.sourceforge.quazip.QuaZIPSource --versions=[0.4.2,0.4.2]')
            if p.strip() == '':
                print('QuaZIPSource 0.4.2 was not found')
            else:
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
                p.wait()
                p = subprocess.Popen("\"" + self._qtsdk + 
                        "\\mingw\\bin\\mingw32-make.exe\"", 
                        cwd="QuaZIP", env=e)
                p.wait()               
                ret = True
        else:            
            ret = True
        
        return ret
                
    def _find_npackdcl(self):
        p = os.environ['NPACKD_CL']
        cl = p + '\\npackdcl.exe'
        if p.strip() == '' or not os.path.exists(cl):
            print('NPACKD_CL does not point to a valid NpackdCL installation path')
            return False
        else:
            self._npackd_cl = cl
            return True

    def _check_mingw(self):
        self._qtsdk = "C:\\QtSDK-1.1.1"
        if not os.path.exists(self._qtsdk + "\\mingw\\bin\\gcc.exe"):
            print('Qt SDK 1.1.1 not found')
            return False
        else:
            return True
       
    def _capture(self, cmd):
        '''Captures the output of a process.
        '''
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        output, errors = p.communicate()
        return output.decode("utf-8", "strict").strip()
        
    def build(self):
        self._project_path = os.path.abspath("")
        
        if not self._check_mingw():
            return
        if not self._find_npackdcl():
            return      
        if not self._build_zlib():
            return      
        if not self._build_quazip():
            return
        if not self._build_mingw_utils():
            return
        
Build().build()
        