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
public:
    App();

    /**
     * Process the command line.
     *
     * @param argc number of arguments
     * @param argv arguments
     * @return exit code
     */
    int process(int argc, char *argv[]);
};

#endif // APP_H
