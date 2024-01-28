#ifndef LOGIN_H
#define LOGIN_H

#include <QDialog>

namespace Ui {
class login;
}

class login : public QDialog
{
    Q_OBJECT

public:
    explicit login(QWidget *parent = nullptr);
    ~login();
public slots:
    void on_button_login_clicked();
    void on_button_sign_clicked();
private:
    Ui::login *ui;
};

#endif // LOGIN_H
