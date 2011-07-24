#include "installoperation.h"

InstallOperation::InstallOperation()
{
    this->install = true;
    this->packageVersion = 0;
}

void InstallOperation::simplify(QList<InstallOperation*> ops)
{
    for (int i = 0; i < ops.size(); i++) {
        InstallOperation* op = ops.at(i);
        if (!op->install) {
            for (int j = i + 1; j < ops.size(); j++) {
                InstallOperation* op2 = ops.at(j);
                if (op->packageVersion == op2->packageVersion &&
                        op2->install) {
                    ops.removeAt(j);
                    break;
                }
            }
        }
    }
}
