#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include "qstring.h"
#include "qstringlist.h"
#include "qlist.h"

/**
 * Parses a command line with options. The option names are case-sensitive.
 */
class CommandLine
{
    class Option {
    public:
        QString name;
        char name2;
        QString valueDescription;
        QString description;
        bool multiple;

        bool nameMathes(const QString& name);
    };

    class ParsedOption {
    public:
        Option* opt;
        QString value;
    };

    QList<Option*> options;
    QList<ParsedOption*> parsedOptions;
    QStringList freeArguments;

    Option* findOption(const QString& name);
    QString processOneParam(QStringList* params);
public:
    /**
     * -
     */
    CommandLine();

    ~CommandLine();

    /**
     * Adds an option.
     *
     * @param name name of the option
     * @param name2 short name or 0 if not available
     * @param description short description of this option for printing help
     * @param valueDescription description of the value for this option.
     *     If "", a value is not possible.
     * @param multiple true if multiple occurences of this option are allowed
     */
    void add(QString name, char name2, QString description, QString valueDescription,
            bool multiple);

    /**
     * Print the option description
     *
     * Example:
     *    -p, --package   package name",
     *    -r, --versions  version range (e.g. [1.1,1.2) )",
     *    -v, --version   version (e.g. 1.2.47)",
     *    -u, --url       repository URL (e.g. https://www.example.com/Rep.xml)",
     *    -s, --status    filters package versions by status",
     */
    void printOptions() const;

    /**
     * Parses program arguments
     *
     * @return error message or ""
     */
    QString parse();

    /**
     * @param name name of the option
     * @return true if the given option is present at least once in the command
     *     line
     */
    bool isPresent(const QString& name);

    /**
     * @param name name of the option
     * @return value of the option or QString::Null if not present
     */
    QString get(const QString& name);

    /**
     * @param name name of the option
     * @return values of the option or an empty list if not present
     */
    QStringList getAll(const QString& name);

    /**
     * @return "free" arguments (those without options)
     */
    QStringList getFreeArguments();
};

#endif // COMMANDLINE_H
