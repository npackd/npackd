#include <windows.h>

#include "qdebug.h"

#include "fileloader.h"
#include "downloader.h"
#include "job.h"

FileLoader::FileLoader()
{
}

void FileLoader::run()
{
    CoInitialize(NULL);
    while (this->terminated != 1) {
        if (this->work.isEmpty())
            Sleep(1000);
        else {
            FileLoaderItem it = this->work.dequeue();
            // qDebug() << "FileLoader::run " << it.url;
            Job* job = new Job();
            it.f = Downloader::download(job, it.url, 0, true);

            emit downloaded(it);
        }
    }
    CoUninitialize();
}
