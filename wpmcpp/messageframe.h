#ifndef MESSAGEFRAME_H
#define MESSAGEFRAME_H

#include <QFrame>

namespace Ui {
    class MessageFrame;
}

class MessageFrame : public QFrame
{
    Q_OBJECT

public:
    explicit MessageFrame(QWidget *parent = 0);
    ~MessageFrame();

    /**
     * @param msg new message
     */
    void setMessage(const QString& msg);
private slots:
    void on_pushButton_clicked();
    void timerTimeout();
private:
    Ui::MessageFrame *ui;
};

#endif // MESSAGEFRAME_H
