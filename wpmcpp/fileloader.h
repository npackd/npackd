#ifndef FILELOADER_H
#define FILELOADER_H

#include "qqueue.h"
#include "qurl.h"
#include "qtemporaryfile.h"


/////////// this code is not yet ready

/**
 * A work item for FileLoader.
 */
class FileLoaderItem {
public:
    /** this file will be downloaded */
    QUrl url;

    /** the downloaded file */
    QTemporaryFile* f;

    /**
     * @param url this file will be downloaded
     */
    FileLoaderItem(const QUrl& url);
};

/**
 * Loads files from the Internet.
 */
class FileLoader
{
public:
    /**
     * Add items to this queue and they will be downloaded.
     */
    QQueue<FileLoaderItem> work;

    /**
     * The thread is not started.
     */
    FileLoader();

    /**
     * Starts the background thread.
     */
    void start();
};

#endif // FILELOADER_H
