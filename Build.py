import os
import subprocess

class Build:
    def _check_installation(self):
        p = os.environ['NPACKD_CL']
        cl = p + '\\npackdcl.exe'
        if p.strip() == '' or not os.path.exists(cl):
            print('NPACKD_CL does not point to a valid NpackdCL installation path')
            return False
        else:
            self._npackd_cl = p
            
        if not os.path.exists(self._qtsdk + "\\mingw\\bin\\gcc.exe"):
            print('Qt SDK not found')
            return False
            
        p = self._capture(cl + ' path --package=net.zlib.ZLibSource --versions=[1.2.5,1.2.5]')
        if p.strip() == '' or not os.path.exists(cl):
            print('zlib 1.2.5 was not found')
            return False
        else:
            self._zlib = p
            
        p = self._capture(cl + ' path --package=net.sourceforge.quazip.QuaZIPSource --versions=[0.4.2,0.4.2]')
        if p.strip() == '' or not os.path.exists(cl):
            print('QuaZIPSource 0.4.2 was not found')
            return False
        else:
            self._quazip = p
            
        return True
        
    def _capture(self, cmd):
        '''Captures the output of a process.
        '''
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        output, errors = p.communicate()
        return output.decode("utf-8", "strict").strip()
        
    def build(self):
        self._qtsdk = "C:\\QtSDK-1.1.1"
    
        if not self._check_installation():
            return      
        
        print("Found zlib 1.2.5 at %s" % self._zlib)
        print("Found QuaZIP 0.4.2 at %s" % self._quazip)

        e = dict(os.environ)
        e["PATH"] = self._qtsdk + "\\mingw\\bin"
        p = subprocess.Popen("\"" + self._qtsdk + 
                "\\mingw\\bin\\mingw32-make.exe\" -f win32\Makefile.gcc", 
                cwd=self._zlib, env=e)
        p.wait()
        
Build().build()
        