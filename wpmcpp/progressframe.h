#ifndef PROGRESSFRAME_H
#define PROGRESSFRAME_H

#include <QFrame>

namespace Ui {
    class ProgressFrame;
}

class ProgressFrame : public QFrame
{
    Q_OBJECT

public:
    explicit ProgressFrame(QWidget *parent = 0);
    ~ProgressFrame();

private:
    Ui::ProgressFrame *ui;
};

#endif // PROGRESSFRAME_H
