#include <windows.h>

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
            Sleep(100);
        else {
            FileLoaderItem it = this->work.dequeue();
            Job* job = new Job();
            it.f = Downloader::download(job, it.url);
            this->done.enqueue(it);
        }
    }
    CoUninitialize();
}
