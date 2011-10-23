#include "installoperation.h"

InstallOperation::InstallOperation()
{
    this->install = true;
    this->packageVersion = 0;
}

void InstallOperation::simplify(QList<InstallOperation*> ops)
{
    for (int i = 0; i < ops.size(); ) {
        InstallOperation* op = ops.at(i);

        int found = -1;
        for (int j = i + 1; j < ops.size(); j++) {
            InstallOperation* op2 = ops.at(j);
            if (op->packageVersion == op2->packageVersion &&
                    op->install != op2->install) {
                found = j;
                break;
            }
        }

        if (found >= 0) {
            delete ops.takeAt(i);
            delete ops.takeAt(found);
        } else {
            i++;
        }
    }
}
