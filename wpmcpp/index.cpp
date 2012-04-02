#include "index.h"

Index::Index(int size)
{
    this->size = size;
}

Index& Index::operator=(const Index& other)
{
    this->size = other.size;
    this->term2doc = other.term2doc;
    return *this;
}

void Index::setText(int index, const QString &text)
{
    QSet<QString> tokens = splitForIndexing(text);
    for (QSet<QString>::const_iterator it = tokens.begin();
            it != tokens.end(); it++) {
        QString term = *it;
        QMap<QString, QBitArray>::iterator ci = term2doc.find(term);
        if (ci != term2doc.end())
            (*ci).setBit(index);
        else {
            QBitArray ba(this->size);
            ba.setBit(index);
            this->term2doc.insert(term, ba);
        }
    }
}

QBitArray Index::find(const QString &query) const
{
    QBitArray r(this->size);
    r.fill(true);

    QStringList terms = splitForSearch(query);
    for (int i = 0; i < terms.count(); i++) {
        QString term = terms.at(i);
        QMap<QString, QBitArray>::const_iterator it = this->term2doc.find(term);
        if (it != this->term2doc.end()) {
            r &= *it;
        } else {
            r.clear();
            break;
        }
    }

    return r;
}

QStringList Index::splitForSearch(const QString& text) const
{
    return text.toLower().simplified().split(" ");
}

QSet<QString> Index::splitForIndexing(const QString& text) const
{
    QStringList words = text.toLower().simplified().split(" ");
    QSet<QString> terms;
    for (int i = 0; i < words.count(); i++) {
        QString word = words.at(i);
        for (int start = 0; start < word.length() - 1; start++) {
            for (int len = word.length() - start; len > 0; len--) {
                terms.insert(word.mid(start, len));
            }
        }
    }
    return terms;
}
