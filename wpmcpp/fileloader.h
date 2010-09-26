#ifndef FILELOADER_H
#define FILELOADER_H

#include "qmetatype.h"
#include "qobject.h"
#include "qqueue.h"
#include "qurl.h"
#include "qtemporaryfile.h"
#include "qthread.h"
#include "qatomic.h"

#include "fileloaderitem.h"

/**
 * Loads files from the Internet.
 */
class FileLoader: public QThread
{
    Q_OBJECT
public:
    /** set this to 1 to terminate this thread. */
    QAtomicInt terminated;

    /**
     * Add items to this queue and they will be downloaded.
     */
    QQueue<FileLoaderItem> work;

    /**
     * Completed items are removed from "work" and placed here.
     */
    QQueue<FileLoaderItem> done;

    /**
     * The thread is not started.
     */
    FileLoader();

    virtual ~FileLoader() {};

    void run();
signals:
    /**
     * This signal will be fired each time something was downloaded or
     * a download ended with an error.
     */
    void downloaded(const FileLoaderItem& it);
};

#endif // FILELOADER_H
