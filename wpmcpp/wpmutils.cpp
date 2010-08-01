#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include "qdebug.h"
#include "qdir.h"
#include "qstring.h"
#include "qfile.h"

#include "wpmutils.h"

/* Task creation
#include <windows.h>
#include <initguid.h>
#include <ole2.h>
#include <mstask.h>
#include <msterr.h>
#include <objidl.h>
#include <wchar.h>
#include <stdio.h>
*/

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

bool WPMUtils::isUnder(QString &file, QString &dir)
{
    QString f = file.replace('/', '\\').toLower();
    QString d = dir.replace('/', '\\').toLower();

    return f.startsWith(d);
}

void WPMUtils::formatMessage(DWORD err, QString* errMsg)
{
    HLOCAL pBuffer;
    DWORD n = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                   FORMAT_MESSAGE_FROM_SYSTEM,
                   0, err, 0, (LPTSTR)&pBuffer, 0, 0);
    if (n == 0)
        errMsg->append(QString("Error %1").arg(err));
    else {
        errMsg->setUtf16((ushort*) pBuffer, n);
        LocalFree(pBuffer);
    }
}


QString WPMUtils::getProgramShortcutsDir()
{
    WCHAR dir[MAX_PATH];
    SHGetFolderPath(0, CSIDL_PROGRAMS, NULL, 0, dir);
    return  QString::fromUtf16(reinterpret_cast<ushort*>(dir));
}

bool WPMUtils::removeDirectory(QDir &aDir, QString *errMsg)
{
    bool ok = true;
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
                ok = removeDirectory(dd, errMsg);
                if (!ok)
                    qDebug() << "PackageVersion::removeDirectory.3" << *errMsg;
            } else {
                QFile file(path);
                ok = file.remove();
                if (!ok && file.exists()) {
                    ok = false;
                    errMsg->clear();
                    errMsg->append("Cannot delete the file: ").append(path);
                    qDebug() << "PackageVersion::removeDirectory.1" << *errMsg;
                }
            }
            if (!ok)
                break;
        }
        if (ok && !aDir.rmdir(aDir.absolutePath())) {
            qDebug() << "PackageVersion::removeDirectory.2";
            ok = false;
            errMsg->clear();
            errMsg->append("Cannot delete the directory: ").append(
                    aDir.absolutePath());
        }
    }
    qDebug() << "PackageVersion::removeDirectory: " << aDir << " " << ok <<
            *errMsg;
    return ok;
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
