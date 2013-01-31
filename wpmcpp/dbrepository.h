#ifndef DBREPOSITORY_H
#define DBREPOSITORY_H

#include <QString>
#include <QSqlError>
#include <QSqlDatabase>

/**
 * @brief A repository stored in an SQLite database.
 */
class DBRepository
{
private:
    static DBRepository def;
    static bool tableExists(QSqlDatabase* db,
            const QString& table, QString* err);
    static QString toString(const QSqlError& e);
public:
    /**
     * @return default repository
     */
    static DBRepository* getDefault();

    /**
     * @brief -
     */
    DBRepository();

    /**
     * @brief opens the database
     * @return error
     */
    QString open();
};

#endif // DBREPOSITORY_H
