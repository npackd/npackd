#ifndef APP_H
#define APP_H

#include "..\wpmcpp\commandline.h"

class App
{
private:
    CommandLine cl;

    int help();
    int addPath();
    int removePath();
    int listMSI();
public:
    App();

    /**
     * Process the command line.
     *
     * @return exit code
     */
    int process();
};

#endif // APP_H
