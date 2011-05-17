#include <iostream>

#include "qdebug.h"

#include "commandline.h"

CommandLine::CommandLine()
{
}

CommandLine::~CommandLine()
{
    qDeleteAll(this->options);
    qDeleteAll(this->parsedOptions);
}

CommandLine::Option* CommandLine::findOption(const QString& name)
{
    Option* r = 0;
    for (int i = 0; i < this->options.count(); i++) {
        Option* opt = this->options.at(i);
        if (opt->name == name) {
            r = opt;
            break;
        }
    }
    return r;
}

void CommandLine::add(QString name, QString description,
        QString valueDescription, bool multiple)
{
    Option* opt = new Option();
    opt->name = name;
    opt->description = description;
    opt->valueDescription = valueDescription;
    opt->multiple = multiple;
    this->options.append(opt);
}

QString CommandLine::parse(int argc, char *argv[])
{
    QString err;

    QStringList params;
    for (int i = 1; i < argc; i++) {
        params.append(QString(argv[i]));
        // std::cout << "Param: " << qPrintable(params.at(params.count() - 1)) << std::endl;
    }

    int i = 0;
    while (i < params.count() && err.isEmpty()) {
        QString p = params.at(i);
        bool nameFound = false;
        bool valueFound = false;
        QString name;
        QString value;

        if (p == "--" || p == "-") {
            err = QString("Missing option name: %1").arg(p);
        } else if (p.startsWith("--")) {
            int pos = p.indexOf("=");
            if (pos == 2) {
                err = QString("Option name expected: %1").arg(p);
            } else if (pos >= 0) {
                name = p.mid(2, pos - 2);
                value = p.right(p.length() - pos - 1);
                nameFound = true;
                valueFound = true;
            } else {
                name = p.right(p.length() - 2);
                nameFound = true;
            }
        } else if (p.startsWith("-")) {
            int pos = p.indexOf("=");
            if (pos == 1) {
                err = QString("Option name cannot start with the equality sign: %1").
                        arg(p);
            } else if (pos > 2) {
                err = QString("Only one-letter options can start with a minus sign: %1").
                        arg(p);
            } else if (pos == 2) {
                name = p.mid(1, 1);
                value = p.right(p.length() - 3);
                nameFound = true;
                valueFound = true;
            } else {
                if (p.length() > 2) {
                    err = QString("Only one-letter options can start with a minus sign: %1").
                            arg(p);
                } else {
                    name = p.mid(2, pos - 2);
                    nameFound = true;
                }
            }
        }
        i++;

        // std::cout << qPrintable(name) << " --> " << qPrintable(value) <<
        //        " : " << qPrintable(err) << nameFound << valueFound << std::endl;

        if (err.isEmpty()) {
            if (nameFound || valueFound) {
                // std::cout << "Searching: " << qPrintable(name) << std::endl;
                Option* opt = findOption(name);
                if (!opt) {
                    err = QString("Unknown option: %1").arg(name);
                } else {
                    if (opt->valueDescription.isEmpty() && valueFound) {
                        err = QString("Unexpected value for the option %1").arg(name);
                    } else {
                        ParsedOption* po = new ParsedOption();
                        po->name = name;
                        this->parsedOptions.append(po);
                        if (valueFound)
                            po->value = value;
                        else {
                            if (!opt->valueDescription.isEmpty()) {
                                if (i >= params.count() || params.at(i).startsWith("-")) {
                                    err = QString("Missing value for the option %1").
                                            arg(name);
                                } else {
                                    po->value = params.at(i);
                                    i++;
                                }
                            }
                        }
                    }
                }
            } else {
                this->freeArguments.append(p);
            }
        }
    }

    return err;
}

bool CommandLine::isPresent(const QString& name)
{
    bool r = false;
    for (int i = 0; i < this->parsedOptions.count(); i++) {
        ParsedOption* po = this->parsedOptions.at(i);
        if (po->name == name) {
            r = true;
            break;
        }
    }
    return r;
}

QString CommandLine::get(const QString& name)
{
    QString r;
    for (int i = 0; i < this->parsedOptions.count(); i++) {
        ParsedOption* po = this->parsedOptions.at(i);
        if (po->name == name) {
            r = po->value;
            break;
        }
    }
    return r;
}

QStringList CommandLine::getFreeArguments()
{
    return this->freeArguments;
}

void CommandLine::dump()
{
    std::cout << "Command line options:" << std::endl;
    std::cout << "Free arguments: " <<
            qPrintable(this->freeArguments.join(", ")) << std::endl;
    for (int i = 0; i < this->parsedOptions.count(); i++) {
        ParsedOption* po = this->parsedOptions.at(i);
        std::cout << "Name: " << qPrintable(po->name) <<
                " Value: " << qPrintable(po->value) << std::endl;
    }
}

QStringList CommandLine::getAll(const QString& name)
{
    QStringList r;
    for (int i = 0; i < this->parsedOptions.count(); i++) {
        ParsedOption* po = this->parsedOptions.at(i);
        if (po->name == name) {
            r.append(po->value);
        }
    }
    return r;
}

