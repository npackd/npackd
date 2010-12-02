#include "fileextensionhandler.h"

FileExtensionHandler::FileExtensionHandler(const QString extension,
        const QString program)
{
    this->program = program;
    this->extension = extension;
}
