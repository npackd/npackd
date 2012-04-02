#ifndef INDEX_H
#define INDEX_H

#include <QString>
#include <QBitArray>
#include <QMap>
#include <QStringList>
#include <QSet>

/**
 * Full-text indexing.
 */
class Index
{
private:
    int size;
    QMap<QString, QBitArray> term2doc;

    QSet<QString> splitForIndexing(const QString& text) const;
    QStringList splitForSearch(const QString& text) const;
public:
    /**
     * @param size number of documents in this index.
     */
    Index(int size);

    Index& operator=(const Index& other);

    /**
     * @param index document index
     * @param text document content
     */
    void setText(int index, const QString& text);

    /**
     * Searches for documents.
     *
     * @param query search query
     * @return found documents
     */
    QBitArray find(const QString& query) const;
};

#endif // INDEX_H
