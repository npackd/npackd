#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <psapi.h>
#include <tlhelp32.h>
#include "msi.h"
#include <shellapi.h>

#include "qdebug.h"
#include "qdir.h"
#include "qstring.h"
#include "qfile.h"
#include <QCryptographicHash>
#include <QFile>
#include "qsettings.h"
#include "qvariant.h"

#include "wpmutils.h"
#include "version.h"

//#include <windows.h>
//#include <initguid.h>
//#include <ole2.h>
//#include <mstask.h>
//#include <msterr.h>
//#include <objidl.h>
//#include <wchar.h>
//#include <stdio.h>

const char* WPMUtils::NPACKD_VERSION = "1.14.1";

WPMUtils::WPMUtils()
{
}

QString WPMUtils::parentDirectory(const QString& path)
{
    QString p = path;
    p = p.replace('/', '\\');
    int index = p.lastIndexOf('\\');
    return p.left(index);
}

QString WPMUtils::getProgramFilesDir()
{
    WCHAR dir[MAX_PATH];
    SHGetFolderPath(0, CSIDL_PROGRAM_FILES, NULL, 0, dir);
    return  QString::fromUtf16(reinterpret_cast<ushort*>(dir));
}

bool WPMUtils::isUnder(const QString &file, const QString &dir)
{
    QString f = file;
    QString d = dir;
    f = f.replace('/', '\\').toLower();
    d = d.replace('/', '\\').toLower();
    if (!d.endsWith('\\'))
        d = d + "\\";

    return f.startsWith(d);
}

void WPMUtils::formatMessage(DWORD err, QString* errMsg)
{
    HLOCAL pBuffer;
    DWORD n;
    if (err >= 12001 && err <= 12156) {
        // wininet.dll-errors
        n = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_HMODULE,
                       GetModuleHandle(L"wininet.dll"),
                       err, 0, (LPTSTR)&pBuffer, 0, 0);
    } else {
        n = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM,
                       0, err, 0, (LPTSTR)&pBuffer, 0, 0);
    }
    if (n == 0)
        errMsg->append(QString("Error %1").arg(err));
    else {
        errMsg->setUtf16((ushort*) pBuffer, n);
        LocalFree(pBuffer);
    }
}

QString WPMUtils::getInstallationDirectory()
{
    QSettings s1(QSettings::SystemScope, "Npackd", "Npackd");
    QString v = s1.value("path", "").toString();
    if (v.isEmpty()) {
        QSettings s(QSettings::SystemScope, "WPM", "Windows Package Manager");
        v = s.value("path", "").toString();
        if (v.isEmpty()) {
            v = WPMUtils::getProgramFilesDir() + "\\WPM";
            QDir dir(v);
            if (!dir.exists())
                v = WPMUtils::getProgramFilesDir() + "\\Npackd";
        }
        s1.setValue("path", v);
    }
    return v;
}

/**
 * see getInstallationDirectory()
 */
void WPMUtils::setInstallationDirectory(const QString& dir)
{
    QSettings s(QSettings::SystemScope, "Npackd", "Npackd");
    s.setValue("path", dir);
}

// see also http://msdn.microsoft.com/en-us/library/ms683217(v=VS.85).aspx
QStringList WPMUtils::getProcessFiles()
{
    QStringList r;

    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);

    // >= Windows Vista
    if (osvi.dwMajorVersion >= 6) {
        BOOL WINAPI (*lpfQueryFullProcessImageName)(
                HANDLE, DWORD, LPTSTR, PDWORD);

        HINSTANCE hInstLib = LoadLibraryA("KERNEL32.DLL");
        lpfQueryFullProcessImageName =
                (BOOL (WINAPI*) (HANDLE, DWORD, LPTSTR, PDWORD))
                GetProcAddress(hInstLib, "QueryFullProcessImageNameW");

        DWORD aiPID[1000], iCb = 1000;
        DWORD iCbneeded;
        if (!EnumProcesses(aiPID, iCb, &iCbneeded)) {
            FreeLibrary(hInstLib);
            return r;
        }

        // How many processes are there?
        int iNumProc = iCbneeded / sizeof(DWORD);

        // Get and match the name of each process
        for (int i = 0; i < iNumProc; i++) {
            // First, get a handle to the process
            HANDLE hProc = OpenProcess(
                    PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                    FALSE, aiPID[i]);

            // Now, get the process name
            if (hProc) {
                HMODULE hMod;
                if (EnumProcessModules(hProc, &hMod, sizeof(hMod), &iCbneeded)) {
                    if (iCbneeded != 0) {
                        HMODULE* modules = new HMODULE[iCbneeded / sizeof(HMODULE)];
                        if (EnumProcessModules(hProc, modules, iCbneeded,
                                &iCbneeded)) {
                            DWORD len = MAX_PATH;
                            WCHAR szName[MAX_PATH];
                            if (lpfQueryFullProcessImageName(hProc, 0, szName,
                                    &len)) {
                                QString s;
                                s.setUtf16((ushort*) szName, len);
                                r.append(s);
                            }
                        }
                        delete[] modules;
                    }
                }
                CloseHandle(hProc);
            }
        }
        FreeLibrary(hInstLib);
    }
    return r;
}

QString WPMUtils::getShellDir(int type)
{
    WCHAR dir[MAX_PATH];
    SHGetFolderPath(0, type, NULL, 0, dir);
    return QString::fromUtf16(reinterpret_cast<ushort*>(dir));
}

Version WPMUtils::getDLLVersion(const QString &path)
{
    Version res(0, 0);

    DWORD dwVerHnd;
    DWORD size;
    size = GetFileVersionInfoSize((LPWSTR) path.utf16(), &dwVerHnd);
    if (size != 0) {
        void* mem = malloc(size);
        if (GetFileVersionInfo((LPWSTR) path.utf16(), 0, size, mem)) {
            VS_FIXEDFILEINFO *pFileInfo;
            unsigned int bufLen;
            if (VerQueryValue(mem, (WCHAR*) L"\\", (LPVOID *) &pFileInfo,
                    &bufLen)) {
                res.setVersion(HIWORD(pFileInfo->dwFileVersionMS),
                        LOWORD(pFileInfo->dwFileVersionMS),
                        HIWORD(pFileInfo->dwFileVersionLS),
                        LOWORD(pFileInfo->dwFileVersionLS));
            }
        }
        free(mem);
    }

    return res;
}

QStringList WPMUtils::findInstalledMSIProducts()
{
    QStringList result;
    WCHAR buf[39];
    int index = 0;
    while (true) {
        UINT r = MsiEnumProducts(index, buf);
        if (r != ERROR_SUCCESS)
            break;
        QString v;
        v.setUtf16((ushort*) buf, 38);
        result.append(v);
        index++;
    }
    return result;
}

void WPMUtils::regDeleteTree(HKEY hkey, const QString path)
{
    const REGSAM KEY_WOW64_64KEY = 0x0100;
    bool w64bit = is64BitWindows();
    HKEY hk;
    if (RegOpenKeyEx(hkey,
            (WCHAR*) path.utf16(),
            0, KEY_ALL_ACCESS | (w64bit ? KEY_WOW64_64KEY : 0),
            &hk) == ERROR_SUCCESS) {
        WCHAR name[255];
        int index = 0;
        while (true) {
            DWORD nameSize = sizeof(name) / sizeof(name[0]);
            LONG r = RegEnumKeyEx(hk, index, name, &nameSize,
                    0, 0, 0, 0);
            if (r == ERROR_SUCCESS) {
                QString v_;
                v_.setUtf16((ushort*) name, nameSize);
                regDeleteTree(hk, v_);
            } else if (r == ERROR_NO_MORE_ITEMS) {
                break;
            }
            index++;
        }
        RegCloseKey(hk);
    }
    RegDeleteKey(hkey, (WCHAR*) path.utf16());
}

QString WPMUtils::regQueryValue(HKEY hk, const QString &var)
{
    QString value_;
    char value[255];
    DWORD valueSize = sizeof(value);
    if (RegQueryValueEx(hk, (WCHAR*) var.utf16(), 0, 0, (BYTE*) value,
            &valueSize) == ERROR_SUCCESS) {
        value_.setUtf16((ushort*) value, valueSize / 2 - 1);
    }
    return value_;
}

QString WPMUtils::sha1(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
         return "";

    QCryptographicHash hash(QCryptographicHash::Sha1);
    char buffer[8192];
    bool err = false;
    while (!file.atEnd()) {
        qint64 r = file.read(buffer, sizeof(buffer));
        if (r < 0) {
            err = true;
            break;
        }
        hash.addData(buffer, r);
    }
    file.close(); // when your done.

    if (err)
        return "";
    else
        return hash.result().toHex();
}

void WPMUtils::removeDirectory2(Job* job, QDir &d)
{
    /* old code
    Job* sub = job->newSubJob(0.3);
    WPMUtils::removeDirectory(sub, d);
    if (!sub->getErrorMessage().isEmpty()) {
        delete sub;
        job->setProgress(0.3);
        Sleep(5000); // 5 Seconds
        job->setProgress(0.6);

        sub = job->newSubJob(0.4);
        WPMUtils::removeDirectory(sub, d);
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(sub->getErrorMessage());
    } else{
        delete sub;
        job->setProgress(1);
    }
    job->complete();
    */

    QString err = WPMUtils::moveToRecycleBin(d.absolutePath());
    d.refresh();
    if (!err.isEmpty() && d.exists()) {
        job->setProgress(0.3);
        Sleep(5000); // 5 Seconds
        job->setProgress(0.6);
        err = WPMUtils::moveToRecycleBin(d.absolutePath());
        d.refresh();
        if (!err.isEmpty() && d.exists())
            job->setErrorMessage(err);
        else
            job->setProgress(1);
    }
    job->complete();
}

QString WPMUtils::getShellFileOperationErrorMessage(int res)
{
    QString r;
    switch (res) {
        case 0:
            r = "";
            break;
        case 0x71:
            r = "The source and destination files are the same file.";
            break;
        case 0x72:
            r = "Multiple file paths were specified in the source buffer, but only one destination file path.";
            break;
        case 0x73:
            r = "Rename operation was specified but the destination path is a different directory. Use the move operation instead.";
            break;
        case 0x74:
            r = "The source is a root directory, which cannot be moved or renamed.";
            break;
        case 0x75:
            r = "The operation was canceled by the user, or silently canceled if the appropriate flags were supplied to SHFileOperation.";
            break;
        case 0x76:
            r = "The destination is a subtree of the source.";
            break;
        case 0x78:
            r = "Security settings denied access to the source.";
            break;
        case 0x79:
            r = "The source or destination path exceeded or would exceed MAX_PATH.";
            break;
        case 0x7A:
            r = "The operation involved multiple destination paths, which can fail in the case of a move operation.";
            break;
        case 0x7C:
            r = "The path in the source or destination or both was invalid.";
            break;
        case 0x7D:
            r = "The source and destination have the same parent folder.";
            break;
        case 0x7E:
            r = "The destination path is an existing file.";
            break;
        case 0x80:
            r = "The destination path is an existing folder.";
            break;
        case 0x81:
            r = "The name of the file exceeds MAX_PATH.";
            break;
        case 0x82:
            r = "The destination is a read-only CD-ROM, possibly unformatted.";
            break;
        case 0x83:
            r = "The destination is a read-only DVD, possibly unformatted.";
            break;
        case 0x84:
            r = "The destination is a writable CD-ROM, possibly unformatted.";
            break;
        case 0x85:
            r = "The file involved in the operation is too large for the destination media or file system.";
            break;
        case 0x86:
            r = "The source is a read-only CD-ROM, possibly unformatted.";
            break;
        case 0x87:
            r = "The source is a read-only DVD, possibly unformatted.";
            break;
        case 0x88:
            r = "The source is a writable CD-ROM, possibly unformatted.";
            break;
        case 0xB7:
            r = "MAX_PATH was exceeded during the operation.";
            break;
        case 0x402:
            r = "An unknown error occurred. This is typically due to an invalid path in the source or destination. This error does not occur on Windows Vista and later.";
            break;
        case 0x10000:
            r = "An unspecified error occurred on the destination.";
            break;
        case 0x10074:
            r = "Destination is a root directory and cannot be renamed.";
            break;
        default:
            WPMUtils::formatMessage(res, &r);
            break;
    }
    return r;
}

QString WPMUtils::moveToRecycleBin(QString dir)
{
    SHFILEOPSTRUCTW f;
    memset(&f, 0, sizeof(f));
    WCHAR from[MAX_PATH + 2];
    wcscpy(from, (WCHAR*) dir.utf16());
    from[wcslen(from) + 1] = 0;
    f.wFunc = FO_DELETE;
    f.pFrom = from;
    f.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI |
            FOF_SILENT | FOF_NOCONFIRMMKDIR;

    int r = SHFileOperationW(&f);
    if (r == 0)
        return "";
    else {
        return QString("Error deleting %1: %2").arg(dir).arg(
                WPMUtils::getShellFileOperationErrorMessage(r));
    }
}

bool WPMUtils::is64BitWindows()
{
    // 32-bit programs run on both 32-bit and 64-bit Windows
    // so must sniff
    WINBASEAPI BOOL WINAPI (*lpfIsWow64Process) (HANDLE,PBOOL);

    HINSTANCE hInstLib = LoadLibraryA("KERNEL32.DLL");
    lpfIsWow64Process =
            (BOOL (WINAPI *) (HANDLE,PBOOL))
            GetProcAddress(hInstLib, "IsWow64Process");
    bool ret;
    if (lpfIsWow64Process) {
        BOOL f64 = FALSE;
        ret = lpfIsWow64Process(GetCurrentProcess(), &f64) && f64;
    } else {
        ret = false;
    }
    FreeLibrary(hInstLib);
    return ret;
}

void WPMUtils::removeDirectory(Job* job, QDir &aDir)
{
    if (aDir.exists()) {
        QFileInfoList entries = aDir.entryInfoList(
                QDir::NoDotAndDotDot |
                QDir::Dirs | QDir::Files);
        int count = entries.size();
        for (int idx = 0; idx < count; idx++) {
            QFileInfo entryInfo = entries[idx];
            QString path = entryInfo.absoluteFilePath();
            if (entryInfo.isDir()) {
                QDir dd(path);
                Job* sub = job->newSubJob(1 / ((double) count + 1));
                removeDirectory(sub, dd);
                if (!sub->getErrorMessage().isEmpty())
                    job->setErrorMessage(sub->getErrorMessage());
                delete sub;
                // if (!ok)
                //    qDebug() << "WPMUtils::removeDirectory.3" << *errMsg;
            } else {
                QFile file(path);
                if (!file.remove() && file.exists()) {
                    job->setErrorMessage(QString("Cannot delete the file: %1").
                            arg(path));
                    // qDebug() << "WPMUtils::removeDirectory.1" << *errMsg;
                } else {
                    job->setProgress(idx / ((double) count + 1));
                }
            }
            if (!job->getErrorMessage().isEmpty())
                break;
        }

        if (job->getErrorMessage().isEmpty()) {
            if (!aDir.rmdir(aDir.absolutePath()))
                // qDebug() << "WPMUtils::removeDirectory.2";
                job->setErrorMessage(QString("Cannot delete the directory: %1").
                        arg(aDir.absolutePath()));
            else
                job->setProgress(1);
        }
    } else {
        job->setProgress(1);
    }

    job->complete();
}

/* Task creation
int main2(int argc, char **argv)
{
  HRESULT hr = S_OK;
  ITaskScheduler *pITS;


  /////////////////////////////////////////////////////////////////
  // Call CoInitialize to initialize the COM library and then
  // call CoCreateInstance to get the Task Scheduler object.
  /////////////////////////////////////////////////////////////////
  hr = CoInitialize(NULL);
  if (SUCCEEDED(hr))
  {
     hr = CoCreateInstance(CLSID_CTaskScheduler,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_ITaskScheduler,
                           (void **) &pITS);
     if (FAILED(hr))
     {
        CoUninitialize();
        return 1;
     }
  }
  else
  {
     return 1;
  }


  /////////////////////////////////////////////////////////////////
  // Call ITaskScheduler::NewWorkItem to create new task.
  /////////////////////////////////////////////////////////////////
  LPCWSTR pwszTaskName;
  ITask *pITask;
  IPersistFile *pIPersistFile;
  pwszTaskName = L"Test Task";

  hr = pITS->NewWorkItem(pwszTaskName,         // Name of task
                         CLSID_CTask,          // Class identifier
                         IID_ITask,            // Interface identifier
                         (IUnknown**)&pITask); // Address of task
                                                                                                                                                                                                                                                                                                                                                                                        //	interface


  pITS->Release();                               // Release object
  if (FAILED(hr))
  {
     CoUninitialize();
     fprintf(stderr, "Failed calling NewWorkItem, error = 0x%x\n",hr);
     return 1;
  }


  /////////////////////////////////////////////////////////////////
  // Call IUnknown::QueryInterface to get a pointer to
  // IPersistFile and IPersistFile::Save to save
  // the new task to disk.
  /////////////////////////////////////////////////////////////////

  hr = pITask->QueryInterface(IID_IPersistFile,
                              (void **)&pIPersistFile);

  pITask->Release();
  if (FAILED(hr))
  {
     CoUninitialize();
     fprintf(stderr, "Failed calling QueryInterface, error = 0x%x\n",hr);
     return 1;
  }


  hr = pIPersistFile->Save(NULL,
                           TRUE);
  pIPersistFile->Release();
  if (FAILED(hr))
  {
     CoUninitialize();
     fprintf(stderr, "Failed calling Save, error = 0x%x\n",hr);
     return 1;
  }


  CoUninitialize();
  printf("Created task.\n");
  return 0;
}
*/
