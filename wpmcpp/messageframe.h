#ifndef MESSAGEFRAME_H
#define MESSAGEFRAME_H

#include <QFrame>

namespace Ui {
    class MessageFrame;
}

class MessageFrame : public QFrame
{
    Q_OBJECT
private:
    QString details;
    bool autoHide;
public:
    explicit MessageFrame(QWidget *parent = 0);
    ~MessageFrame();

    /**
     * @param msg new message
     */
    void setMessage(const QString& msg);

    /**
     * @param msg new details
     */
    void setDetails(const QString& msg);

    /**
     * @param b true = automatically hide the message after some time
     */
    void setAutoHide(bool b);
private slots:
    void timerTimeout();
    void on_pushButtonDetails_clicked();
    void on_pushButtonDismiss_clicked();
private:
    Ui::MessageFrame *ui;
};

#endif // MESSAGEFRAME_H
