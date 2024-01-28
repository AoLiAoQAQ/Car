#include "sign.h"
#include "ui_sign.h"
#include "login.h"

#include <QMessageBox>
#include <QSqlQuery>
sign::sign(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::sign)
{
    ui->setupUi(this);
    setWindowTitle("注册");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    resize(600,400);
}

sign::~sign()
{
    delete ui;
}

void sign::on_button_enter_clicked()
{
    QString user = ui->lineEdit_loguser->text();
    QString password1 = ui->lineEdit_logpw->text();
    QString password2 = ui->lineEdit_logpw_2->text();
    if((user == "" || password1 == "" || password2 == ""))
        QMessageBox::information(this,"错误","用户名或者密码不能为空");
    else if(password2 != password1)
        QMessageBox::information(this,"错误","密码不一致");
    else if((password2 == password1) && user != ""){
        QString insertinfo = QString("insert into info (username,password) values('%1','%2');").arg(user).arg(password1);
        QString selectinfo = QString("select * from info where username = '%1'").arg(user);
        QSqlQuery query;
        query.exec(selectinfo);
        if(query.first()){
            QMessageBox::warning(this,"ERROR","用户名重复");
        }else if(query.exec(insertinfo)){
            QMessageBox::information(this,"成功","注册成功!",QMessageBox::Yes);
        }else{
            QMessageBox::warning(this,"ERROR","注册失败，请重试!");
        }
    }
}


void sign::on_button_return_clicked()
{
    login *log = new login;
    this->close();
    log->exec();

    delete this;
}

