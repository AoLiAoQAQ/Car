#ifndef CARPLATE_H
#define CARPLATE_H

#include "worker.h"
#include "login.h"

#include <QWidget>

#include <QSqlDatabase>
#include <QSqlQuery>

#include <QCamera>
#include <QCameraViewfinder>
#include <QCameraImageCapture>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QSslConfiguration>
#include <QNetworkReply>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QImage>
#include <QThread>
#include <QJsonArray>
#include <QPainter>
#include <QEventLoop>
#include <QCameraInfo>
QT_BEGIN_NAMESPACE
namespace Ui { class carplate; }
QT_END_NAMESPACE

class carplate : public QWidget
{
    Q_OBJECT
signals:
    void beiginwork(QImage img,QThread*childThread);
public:
    carplate(QWidget *parent = nullptr);
    ~carplate();

    QString colorConversion(const QString&color);

    void lockdialog();
    void unlockdialog();
    void clearRectAndInfo();
    void updateText();

    void insertToSql();
    void loadExistingRecords();
    void updateCamera(const QString &deviceName);
    void showTableInfo();
public slots:
    void showCamera(int id,QImage img);
    void takePicture();
    void tokenReply(QNetworkReply* reply);
    void beginNetwork(QByteArray postData,QThread*overThread);
    void imgReply(QNetworkReply* reply);

    void prePostData();

    void searchCarInfo();
    void handleCameraSelection(int index);
    void buttonSelect();
    void LocalPoint(QNetworkReply *reply);
    void LocalMap(QNetworkReply *reply);
private slots:
    void on_button_close_clicked();

    void on_button_table_clicked();

    void on_button_camerainfo_clicked();

    void on_button_exit_clicked();

private:
    Ui::carplate *ui;
    QCamera* camera;
    QCameraViewfinder* finder;
    QCameraImageCapture* imageCapture;
    QTimer*refreshTime;
    QTimer*netTimer;
    QNetworkAccessManager*tokenManager;
    QNetworkAccessManager*imgManager;
    QSslConfiguration sslconfig;
    QString accessToken;
    QImage img;
    QThread* childThread;
    bool prePostDataRunning;
    QEventLoop eventLoop;
    bool iscar;
    int cartop;
    int carleft;
    int carwidth;
    int carheight;
    //QDateTime leaveTime;
    QString carNumerTemp;
    QString carColorTemp;
    QString carnumber;
    QString carcolor;

    QDateTime lastEnterTime;
    QDateTime lastExitTime;
    QString lastcarNmber;

    //地图
    QNetworkAccessManager*localPoint;
    QNetworkAccessManager*localMap;
    QString addr;
    QString x;
    QString y;
};
#endif // CARPLATE_H
