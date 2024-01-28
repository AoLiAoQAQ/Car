#include "carplate.h"

#include <QApplication>

bool connectSql(){
    QSqlDatabase db;
    db = QSqlDatabase::addDatabase("QODBC");
    db.setHostName("127.0.0.1");
    db.setPort(3306);
    db.setDatabaseName("parking");
    db.setUserName("root");
    db.setPassword("root");
    if(db.open()){
        qDebug()<<"connect ok";
    }else{
        qDebug()<<"connect failed";
    }
    // 创建 info 表
    QSqlQuery query(db);
    QString createInfoTableQuery = "CREATE TABLE IF NOT EXISTS info ("
                                   "id INT AUTO_INCREMENT PRIMARY KEY,"
                                   "username VARCHAR(20),"
                                   "password VARCHAR(20)"
                                   ")";
    if (query.exec(createInfoTableQuery)) {
        qDebug() << "Table info created successfully.";
    } else {
        qDebug() << "Error creating table info";
    }

    // 创建 car_info 表
    QString createCarInfoTableQuery = "CREATE TABLE IF NOT EXISTS car_info ("
                                      "id INT AUTO_INCREMENT PRIMARY KEY,"
                                      "car_number VARCHAR(20),"
                                      "car_color VARCHAR(20),"
                                      "enter_time DATETIME,"
                                      "exit_time DATETIME"
                                      ")";
    if (query.exec(createCarInfoTableQuery)) {
        qDebug() << "Table car_info created successfully.";
    } else {
        qDebug() << "Error creating table car_info";
    }

    return true;
//    QSqlQuery query(db);
//    query.exec("SHOW TABLES LIKE 'info'");
//    if(!query.next()){
//        //query.exec("SET NAMES 'Latin1'");
//        query.exec("create table info (id INT AUTO_INCREMENT PRIMARY KEY, username VARCHAR(20), password VARCHAR(20))");
//    }
//    return true;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    login log;
    connectSql();
    log.show();
    return a.exec();
//    login log;
//    connectSql();
//    int res = log.exec();
//    if(res == QDialog::Accepted){
//        carplate w;
//        w.show();
//        return a.exec();
//    }else{
//        return 0;
//    }

}
