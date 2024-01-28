#ifndef SIGN_H
#define SIGN_H

#include <QDialog>

namespace Ui {
class sign;
}

class sign : public QDialog
{
    Q_OBJECT

public:
    explicit sign(QWidget *parent = nullptr);
    ~sign();

private slots:
    void on_button_enter_clicked();

    void on_button_return_clicked();

private:
    Ui::sign *ui;
};

#endif // SIGN_H
