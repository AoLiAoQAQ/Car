#include "login.h"
#include "ui_login.h"
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QMessageBox>
#include "carplate.h"
#include "sign.h"
login::login(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::login)
{
    ui->setupUi(this);
    setWindowTitle("登录");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    resize(600,400);
}

login::~login()
{
    delete ui;
}

void login::on_button_login_clicked()
{
    QString user = ui->lineEdit_loguser->text();
    QString pw = ui->lineEdit_logpw->text();
    QSqlQuery que;
    que.prepare("SELECT * FROM info WHERE username = :username AND password = :password");
    que.bindValue(":username",user);
    que.bindValue(":password",pw);
    que.exec();
    if(que.next()){
        this->close();
        carplate* main = new carplate();
        main->show();
        main->lockdialog();
        if(main->close())
            main->unlockdialog();

    }else{
        QMessageBox::information(this,"登录失败","用户名或密码错误");
        ui->lineEdit_loguser->clear();
        ui->lineEdit_loguser->clear();
    }

}


void login::on_button_sign_clicked()
{
    sign *si = new sign;

    this->close();
    si->exec();

    delete this;
}

