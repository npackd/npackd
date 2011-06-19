#include <iostream>

#include "qdebug.h"

#include "commandline.h"

bool CommandLine::Option::nameMathes(const QString& name)
{
    bool r;
    if (name.length() == 1 && name.at(0) == name2) {
        r = true;
    } else if (this->name == name) {
        r = true;
    } else {
        r = false;
    }

    return r;
}

CommandLine::CommandLine()
{
}

CommandLine::~CommandLine()
{
    qDeleteAll(this->options);
    qDeleteAll(this->parsedOptions);
}

QString CommandLine::processOneParam(QStringList* params)
{
    QString err;

    QString p = params->at(0);
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
            params->removeAt(0);
        } else {
            name = p.right(p.length() - 2);
            nameFound = true;
            params->removeAt(0);
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
            params->removeAt(0);
        } else {
            if (p.length() > 2) {
                err = QString("Only one-letter options can start with a minus sign: %1").
                        arg(p);
            } else {
                name = p.mid(1, 1);
                nameFound = true;
                params->removeAt(0);
            }
        }
    }

    // std::cout << qPrintable(name) << " --> " << qPrintable(value) <<
    //        " : " << qPrintable(err) << nameFound << valueFound << std::endl;

    if (err.isEmpty()) {
        if (nameFound) {
            // std::cout << "Searching: " << qPrintable(name) << std::endl;
            Option* opt = findOption(name);
            if (!opt) {
                err = QString("Unknown option: %1").arg(name);
            } else {
                if (opt->valueDescription.isEmpty() && valueFound) {
                    err = QString("Unexpected value for the option %1").arg(name);
                } else {
                    ParsedOption* po = new ParsedOption();
                    po->opt = opt;
                    this->parsedOptions.append(po);
                    if (valueFound)
                        po->value = value;
                    else {
                        if (!opt->valueDescription.isEmpty()) {
                            if (params->count() == 0) {
                                err = QString("Missing value for the option %1").
                                        arg(name);
                            } else {
                                po->value = params->at(0);
                                params->removeAt(0);
                            }
                        }
                    }
                }
            }
        } else {
            this->freeArguments.append(p);
            params->removeAt(0);
        }
    }

    return err;
}

CommandLine::Option* CommandLine::findOption(const QString& name)
{
    Option* r = 0;
    for (int i = 0; i < this->options.count(); i++) {
        Option* opt = this->options.at(i);
        if (opt->nameMathes(name)) {
            r = opt;
            break;
        }
    }
    return r;
}

void CommandLine::add(QString name, char name2, QString description,
        QString valueDescription, bool multiple)
{
    Option* opt = new Option();
    opt->name = name;
    opt->name2 = name2;
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

    while (params.count() > 0) {
        err = processOneParam(&params);
        if (!err.isEmpty())
            break;
    }

    return err;
}

bool CommandLine::isPresent(const QString& name)
{
    bool r = false;
    for (int i = 0; i < this->parsedOptions.count(); i++) {
        ParsedOption* po = this->parsedOptions.at(i);
        if (po->opt->nameMathes(name)) {
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
        if (po->opt->nameMathes(name)) {
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
        std::cout << "Name: " << qPrintable(po->opt->name) <<
                " Value: " << qPrintable(po->value) << std::endl;
    }
}

QStringList CommandLine::getAll(const QString& name)
{
    QStringList r;
    for (int i = 0; i < this->parsedOptions.count(); i++) {
        ParsedOption* po = this->parsedOptions.at(i);
        if (po->opt->nameMathes(name)) {
            r.append(po->value);
        }
    }
    return r;
}

