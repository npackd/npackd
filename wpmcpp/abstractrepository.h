#ifndef ABSTRACTREPOSITORY_H
#define ABSTRACTREPOSITORY_H

#include <QObject>

class AbstractRepository : public QObject
{
    Q_OBJECT
public:
    explicit AbstractRepository(QObject *parent = 0);

signals:

public slots:

};

#endif // ABSTRACTREPOSITORY_H
