#include "carplate.h"
#include "ui_carplate.h"
#include <QCamera>
#include <QCameraViewfinder>
#include <QCameraImageCapture>
#include <QHBoxLayout>
#include <QDebug>
#include <QByteArray>
#include <QBuffer>
#include <QPainter>
#include <QFont>
#include <QIcon>
#include <QDate>
#include <QTime>

carplate::carplate(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::carplate)
{
    ui->setupUi(this);
    this->resize(1800,1000);
    setWindowTitle("GGBond的停车管理系统");
    camera = nullptr;
    finder = nullptr;
    imageCapture = nullptr;
    refreshTime = nullptr;
    netTimer = nullptr;
    tokenManager = nullptr;
    imgManager = nullptr;
    childThread = nullptr;
    ui->stackedWidget->setCurrentIndex(0);

    localPoint = new QNetworkAccessManager(this);
    connect(localPoint,&QNetworkAccessManager::finished,this,&carplate::LocalPoint);

    QUrl PointUrl("https://api.map.baidu.com/location/ip");
    QUrlQuery PointQue;
    PointQue.addQueryItem("ip","");
    PointQue.addQueryItem("coor","bd09ll");
    PointQue.addQueryItem("ak","MdlgNwYGxNrVU0i7pjXobK3XinAsNjkp");
    PointUrl.setQuery(PointQue);

    sslconfig = QSslConfiguration::defaultConfiguration();
    sslconfig.setPeerVerifyMode(QSslSocket::QueryPeer);
    sslconfig.setProtocol(QSsl::TlsV1_2);

    QNetworkRequest PointReq;
    PointReq.setUrl(PointUrl);
    PointReq.setSslConfiguration(sslconfig);
    localPoint->get(PointReq);

    connect(ui->button_open,&QPushButton::clicked,this,&carplate::prePostData);

    int countcolumn = 4;
    ui->tableWidget_carinfo->setColumnCount(countcolumn);
    ui->tableWidget_carinfo->setHorizontalHeaderLabels({"车牌号","颜色","进入时间","离开时间"});
    ui->tableWidget_carinfo->setColumnWidth(0,100);
    ui->tableWidget_carinfo->setColumnWidth(1,75);
    ui->tableWidget_carinfo->setColumnWidth(2,180);
    ui->tableWidget_carinfo->setColumnWidth(3,180);

    QDate currendate = QDate::currentDate();
    QString datestr = currendate.toString("yyyy<br>MM-dd");
    ui->label_date->setText("<html>" + datestr + "</html>");
    //定时器时间显示
    QTimer *time = new QTimer(this);
    connect(time,&QTimer::timeout,[=](){
        QTime currentime = QTime::currentTime();
        QString timestr = currentime.toString("hh:mm:ss");

        ui->label_time->setText(timestr);
    });
    time->start(1000);
    //    connect(time,&QTimer::timeout,[=](){
    //        QTime currentime = QTime::currentTime();

    //        ui->dateTimeEdit->setTime(currentime);
    //    });
    //    connect(time,&QTimer::timeout,[=](){
    //        QTime currentime = QTime::currentTime();

    //        ui->dateTimeEdit_2->setTime(currentime);
    //    });


    QDateTime datetime = QDateTime::currentDateTime();
    ui->dateTimeEdit->setDateTime(datetime);
    ui->dateTimeEdit_2->setDateTime(datetime);

    ui->tableWidget_select->setColumnCount(countcolumn);
    ui->tableWidget_select->setHorizontalHeaderLabels({"车牌号","颜色","进入时间","离开时间"});
    ui->tableWidget_select->setColumnWidth(0,250);
    ui->tableWidget_select->setColumnWidth(1,150);
    ui->tableWidget_select->setColumnWidth(2,350);
    ui->tableWidget_select->setColumnWidth(3,350);

    connect(ui->lineEdit_carnumselect,&QLineEdit::returnPressed,this,&carplate::searchCarInfo);
    //loadExistingRecords();

    //combobox摄像信息
    QList<QCameraInfo>cameras = QCameraInfo::availableCameras();
    for(const QCameraInfo& cameraInfo : cameras){
        ui->comboBox_selectcam->addItem(cameraInfo.description(),QVariant(cameraInfo.deviceName()));
    }

    connect(ui->comboBox_selectcam,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&carplate::handleCameraSelection);

    connect(ui->button_select,&QPushButton::clicked,this,&carplate::buttonSelect);

}

carplate::~carplate()
{
    delete ui;

}

void carplate::showCamera(int id, QImage img)
{
    Q_UNUSED(id);
    this->img = img;
    if(!iscar){
        ui->label->setPixmap(QPixmap::fromImage(img));
        return;
    }

    QPainter p(&img);
    p.setOpacity(1.0);
    QFont font = p.font();
    font.setPixelSize(30);
    p.setFont(font);
    p.setPen(Qt::red);
    p.drawRect(carleft,cartop,carwidth,carheight);
    p.drawText(carleft+carwidth+5,cartop + 40,QString("车牌:").append(carnumber));
    p.drawText(carleft+carwidth+5,cartop + 80,QString("颜色:").append(carcolor));

    ui->label->setPixmap(QPixmap::fromImage(img));

    carNumerTemp = carnumber;
    carColorTemp = carcolor;
    updateText();

}
void carplate::updateText()
{
    QString carnum = carNumerTemp;
    QString carcol = carColorTemp;

    ui->lineEdit_num->setText(carnum);
    ui->lineEdit_color->setText(carcol);

}

void carplate::insertToSql()
{
    //    ui->tableWidget_select->clearContents();
    //    ui->tableWidget_select->setRowCount(0);
    QString carnum = carnumber;
    QString carcol = carcolor;
    QSqlQuery que;

    // 获取当前时间
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString enterTime = currentDateTime.toString("yyyy-MM-dd hh:mm:ss");
    QString exitTimeStr = "";

    // 查询数据库中是否已存在相同车牌号的记录
    que.prepare("SELECT * FROM car_info WHERE car_number = :car_number AND exit_time IS NULL");
    que.bindValue(":car_number", carnum);

    if (que.exec() && que.next()) {
        // 存在记录，表示车辆已经进入过，更新离开时间
        QString originalEnterTime = que.value("enter_time").toString();
        que.prepare("UPDATE car_info SET exit_time = :exit_time WHERE car_number = :car_number AND exit_time IS NULL");
        que.bindValue(":exit_time", QDateTime::currentDateTime());
        que.bindValue(":car_number", carnum);

        if (que.exec()) {
            qDebug() << "成功更新离开时间。";
            exitTimeStr = currentDateTime.toString("yyyy-MM-dd hh:mm:ss");
        } else {
            qDebug() << "更新离开时间时出错:";
        }
    } else {
        // 不存在记录，表示车辆首次进入，执行插入操作
        que.prepare("INSERT INTO car_info (car_number, car_color, enter_time, exit_time) VALUES (:car_number, :car_color, :enter_time, :exit_time)");
        que.bindValue(":car_number", carnum);
        que.bindValue(":car_color", carcol);
        que.bindValue(":enter_time", QDateTime::currentDateTime());
        que.bindValue(":exit_time", QVariant(QVariant::DateTime));

        if (que.exec()) {
            qDebug() << "成功将数据插入 car_info 表。";
        } else {
            qDebug() << "将数据插入 car_info 表时出错:";
        }
    }

    // 更新表格显示
    int rowCount = ui->tableWidget_carinfo->rowCount();
    int rowIndex = -1;

    // 寻找对应车牌号的行
    for (int i = 0; i < rowCount; ++i) {
        QTableWidgetItem* item = ui->tableWidget_carinfo->item(i, 0);
        QTableWidgetItem* leaveTimeItem = ui->tableWidget_carinfo->item(i, 3);
        if (item && leaveTimeItem && item->text() == carnum && leaveTimeItem->text().isEmpty()) {
            rowIndex = i;
            break;
        }
    }

    if (rowIndex != -1) {
        // 找到对应车牌号的行，并且该车辆未离开
        QTableWidgetItem* leaveTimeItem = ui->tableWidget_carinfo->item(rowIndex, 3);
        if (leaveTimeItem) {
            leaveTimeItem->setText(exitTimeStr);
        }
    } else {
        // 否则，在表格的末尾插入新行
        ui->tableWidget_carinfo->insertRow(rowCount);
        QTableWidgetItem *carnumtable = new QTableWidgetItem(carnum);
        QTableWidgetItem *carcolortable = new QTableWidgetItem(carcol);
        QTableWidgetItem *carEnterTimetable = new QTableWidgetItem(enterTime);
        QTableWidgetItem *carLeaveTimetable = new QTableWidgetItem(exitTimeStr);
        ui->tableWidget_carinfo->setItem(rowCount, 0, carnumtable);
        ui->tableWidget_carinfo->setItem(rowCount, 1, carcolortable);
        ui->tableWidget_carinfo->setItem(rowCount, 2, carEnterTimetable);
        ui->tableWidget_carinfo->setItem(rowCount, 3, carLeaveTimetable);
    }
}

//void carplate::loadExistingRecords()
//{
//    QSqlQuery query;
//    query.prepare("SELECT car_number, car_color, enter_time, exit_time FROM car_info");
//    if (query.exec()) {
//        ui->tableWidget_select->clearContents();
//        ui->tableWidget_select->setRowCount(0);
//        while (query.next()) {
//            QString carNumber = query.value(0).toString();
//            QString carColor = query.value(1).toString();
//            QString enterTime = query.value(2).toDateTime().toString("yyyy-MM-dd hh:mm:ss");
//            QString exitTime = query.value(3).toDateTime().toString("yyyy-MM-dd hh:mm:ss");

//            int rowCount = ui->tableWidget_carinfo->rowCount();
//            ui->tableWidget_carinfo->insertRow(rowCount);
//            QTableWidgetItem *itemCarNumber = new QTableWidgetItem(carNumber);
//            QTableWidgetItem *itemCarColor = new QTableWidgetItem(carColor);
//            QTableWidgetItem *itemEnterTime = new QTableWidgetItem(enterTime);
//            QTableWidgetItem *itemExitTime = new QTableWidgetItem(exitTime);
//            ui->tableWidget_carinfo->setItem(rowCount, 0, itemCarNumber);
//            ui->tableWidget_carinfo->setItem(rowCount, 1, itemCarColor);
//            ui->tableWidget_carinfo->setItem(rowCount, 2, itemEnterTime);
//            ui->tableWidget_carinfo->setItem(rowCount, 3, itemExitTime);
//        }
//    } else {
//        qDebug() << "Error executing SQL query: ";
//    }
//}



void carplate::searchCarInfo()
{
    QString searchCarNumber = ui->lineEdit_carnumselect->text();

    QSqlQuery que;
    if(searchCarNumber.isEmpty()){
        que.prepare("SELECT car_number, car_color, enter_time, exit_time FROM car_info");
    }else{
        que.prepare("SELECT car_number, car_color, enter_time, exit_time FROM car_info WHERE car_number LIKE :car_number");
        que.bindValue(":car_number", "%" + searchCarNumber + "%");
    }

    if (que.exec()) {
        ui->tableWidget_select->clearContents();
        ui->tableWidget_select->setRowCount(0);

        int row = 0;
        while (que.next()) {
            QString carNumber = que.value(0).toString();
            QString carColor = que.value(1).toString();
            QString enterTime = que.value(2).toDateTime().toString("yyyy-MM-dd hh:mm:ss");
            QString exitTime = que.value(3).toDateTime().toString("yyyy-MM-dd hh:mm:ss");

            ui->tableWidget_select->insertRow(row);

            QTableWidgetItem *itemCarNumber = new QTableWidgetItem(carNumber);
            QTableWidgetItem *itemCarColor = new QTableWidgetItem(carColor);
            QTableWidgetItem *itemEnterTime = new QTableWidgetItem(enterTime);
            QTableWidgetItem *itemExitTime = new QTableWidgetItem(exitTime);

            ui->tableWidget_select->setItem(row, 0, itemCarNumber);
            ui->tableWidget_select->setItem(row, 1, itemCarColor);
            ui->tableWidget_select->setItem(row, 2, itemEnterTime);
            ui->tableWidget_select->setItem(row, 3, itemExitTime);

            row++;
        }
    } else {
        qDebug()<<"error to search";
    }
}

void carplate::handleCameraSelection(int index)
{
    if(index >= 0 ){
        QString selectedDevice = ui->comboBox_selectcam->itemData(index).toString();
        updateCamera(selectedDevice);
    }
}

void carplate::buttonSelect()
{
    // 获取车牌号过滤条件
    QString carNumFilter = ui->lineEdit_carnumselect->text();

    QDateTime startTime = ui->dateTimeEdit->dateTime();
    QDateTime endTime = ui->dateTimeEdit_2->dateTime();

    // 根据车牌和时间范围进行查询
    QSqlQuery query;
    query.prepare("SELECT car_number, car_color, enter_time, exit_time FROM car_info "
                  "WHERE car_number LIKE :car_number AND exit_time IS NOT NULL "
                  "AND enter_time >= :enter_time AND exit_time <= :exit_time");

    query.bindValue(":car_number", "%" + carNumFilter + "%");
    query.bindValue(":enter_time", startTime);
    query.bindValue(":exit_time", endTime);

    if (query.exec()) {
        ui->tableWidget_select->clearContents();
        ui->tableWidget_select->setRowCount(0);

        int row = 0;
        while (query.next()) {
            QString carNumber = query.value(0).toString();
            QString carColor = query.value(1).toString();
            QString enterTime = query.value(2).toDateTime().toString("yyyy-MM-dd hh:mm:ss");
            QString exitTime = query.value(3).toDateTime().toString("yyyy-MM-dd hh:mm:ss");

            ui->tableWidget_select->insertRow(row);

            QTableWidgetItem *itemCarNumber = new QTableWidgetItem(carNumber);
            QTableWidgetItem *itemCarColor = new QTableWidgetItem(carColor);
            QTableWidgetItem *itemEnterTime = new QTableWidgetItem(enterTime);
            QTableWidgetItem *itemExitTime = new QTableWidgetItem(exitTime);

            ui->tableWidget_select->setItem(row, 0, itemCarNumber);
            ui->tableWidget_select->setItem(row, 1, itemCarColor);
            ui->tableWidget_select->setItem(row, 2, itemEnterTime);
            ui->tableWidget_select->setItem(row, 3, itemExitTime);

            row++;
        }
    } else {
        qDebug() << "Error executing SQL query: ";
    }
}

void carplate::LocalPoint(QNetworkReply *reply)
{
    if(reply->error()!= QNetworkReply::NoError){
        qDebug()<<reply->errorString();
        return;
    }
    const QByteArray reply_data = reply->readAll();
    //qDebug()<<reply_data;
    QJsonParseError jsonerr;
    QJsonDocument doc = QJsonDocument::fromJson(reply_data,&jsonerr);

    if(jsonerr.error == QJsonParseError::NoError){
        QJsonObject obj = doc.object();
        if(obj.contains("content")){
            QJsonObject con = obj.value("content").toObject();
            if(con.contains("address")){
                addr = con.take("address").toString();
                ui->label_city->setText(addr);
            }
            if(con.contains("point")){
                QJsonObject poi = con.value("point").toObject();
                if(poi.contains("x")&& poi.contains("y")){
                    x = poi.take("x").toString();
                    y = poi.take("y").toString();
                    QString str;
                    str.append("X坐标:").append(x).append("\n").append("Y坐标:").append(y);
                    ui->label_point->setText(str);
                }
            }
        }
    }
    localMap = new QNetworkAccessManager(this);
    connect(localMap,&QNetworkAccessManager::finished,this,&carplate::LocalMap);
    QUrl mapurl("https://api.map.baidu.com/staticimage/v2");
    QUrlQuery mapque;
    mapque.addQueryItem("ak","9y9ebdB70KiYtnXu1FKLDE0BDUdwbnsY");
    mapque.addQueryItem("width","400");
    mapque.addQueryItem("height","300");
    mapque.addQueryItem("center",x +","+ y);
    mapque.addQueryItem("zoom","11");
    mapque.addQueryItem("scale","2");
    mapque.addQueryItem("markers",x +","+ y);
    mapurl.setQuery(mapque);
    QNetworkRequest mapReq;
    mapReq.setUrl(mapurl);
    mapReq.setSslConfiguration(sslconfig);
    localMap->get(mapReq);
    reply->deleteLater();
}

void carplate::LocalMap(QNetworkReply *reply)
{
    if(reply->error()!= QNetworkReply::NoError){
        qDebug()<<reply->errorString();
        return;
    }
    // 处理网络响应的数据
    QByteArray imageData = reply->readAll();
    //qDebug()<<imageData;

    QPixmap pixmap;
    pixmap.loadFromData(imageData);
    ui->label_map->setPixmap(pixmap);

    reply->deleteLater();
}

void carplate::updateCamera(const QString &deviceName)
{
    if(camera != nullptr){
        camera->stop();
        delete camera;
        camera = nullptr;
    }
    camera = new QCamera(deviceName.toUtf8());
    camera->setViewfinder(finder);
    camera->setCaptureMode(QCamera::CaptureStillImage);
    imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);

    connect(imageCapture,&QCameraImageCapture::imageCaptured,this,&carplate::showCamera);
    camera->start();
    clearRectAndInfo();
    refreshTime->start(50);
    netTimer->start(8000);
    prePostData();
}

void carplate::takePicture()
{
    imageCapture->capture();
}

void carplate::tokenReply(QNetworkReply *reply)
{
    if(reply->error()!= QNetworkReply::NoError){
        qDebug()<<reply->errorString();
        return;
    }
    //正常应答
    const QByteArray reply_data = reply->readAll();
    //qDebug()<<reply_data;
    //json解析
    QJsonParseError jsonErr;
    QJsonDocument doc = QJsonDocument::fromJson(reply_data,&jsonErr);

    //解析成功
    if(jsonErr.error == QJsonParseError::NoError){
        QJsonObject obj = doc.object();
        if(obj.contains("access_token")){
            accessToken = obj.take("access_token").toString();
        }
        //ui->textBrowser->setText(accessToken);
    }else{
        qDebug()<<"JSON ERR:"<<jsonErr.errorString();
    }

    reply->deleteLater();
    netTimer->start(8000);
    //prePostData();
}

void carplate::prePostData(){
    clearRectAndInfo();
    if(finder == nullptr && camera == nullptr){
        camera = new QCamera(this);
        finder = new QCameraViewfinder();
        imageCapture = new QCameraImageCapture(camera);
        camera->setViewfinder(finder);

        camera->setCaptureMode(QCamera::CaptureStillImage);
        imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);

        connect(imageCapture,&QCameraImageCapture::imageCaptured,this,&carplate::showCamera);
        camera->start();

        clearRectAndInfo();
        //定时器刷新拍照界面
        refreshTime = new QTimer(this);
        connect(refreshTime,&QTimer::timeout,this,&carplate::takePicture);
        refreshTime->start(50);

        //定时器不断进行车牌识别请求
        netTimer = new QTimer(this);
        connect(netTimer,&QTimer::timeout,this,&carplate::prePostData);

        tokenManager = new QNetworkAccessManager(this);
        connect(tokenManager,&QNetworkAccessManager::finished,this,&carplate::tokenReply);
        //qDebug()<<tokenManager->supportedSchemes();

        imgManager = new QNetworkAccessManager(this);
        connect(imgManager,&QNetworkAccessManager::finished,this,&carplate::imgReply);

        //拼接URL和参数
        QUrl url("https://aip.baidubce.com/oauth/2.0/token");
        QUrlQuery query;
        query.addQueryItem("grant_type","client_credentials");
        query.addQueryItem("client_id","OEznuP4QE4dH98mKDGaLYbHG");
        query.addQueryItem("client_secret","CI53AjzencEIcS6LxcqhIvaZZ0eXce6G");
        url.setQuery(query);

        //    qDebug()<<url;
        //    if(QSslSocket::supportsSsl())
        //        qDebug()<<"支持";
        //    else
        //        qDebug()<<"不支持";

        //ssl配置
        sslconfig = QSslConfiguration::defaultConfiguration();
        sslconfig.setPeerVerifyMode(QSslSocket::QueryPeer);
        sslconfig.setProtocol(QSsl::TlsV1_2);

        QNetworkRequest req;
        req.setUrl(url);
        req.setSslConfiguration(sslconfig);

        tokenManager->get(req);
    }
    //创建子线程，创建工人，把工人送到子线程，绑定信号和槽，启动子线程，给工人发通知干活
    childThread = new QThread(this);
    Worker* worker = new Worker;
    worker->moveToThread(childThread);

    connect(this,&carplate::beiginwork,worker,&Worker::dowork);
    connect(worker,&Worker::resultReady,this,&carplate::beginNetwork);
    connect(childThread,&QThread::finished,worker,&QObject::deleteLater);

    childThread->start();
    emit beiginwork(img,childThread);
}



void carplate::beginNetwork(QByteArray postData,QThread*overThread)
{
    //关闭子线程
    overThread->exit();
    overThread->wait();
    if(childThread->isFinished())
        qDebug()<<"子线程结束了";
    else
        qDebug()<<"子线程没有结束";


    // 组装车牌识别请求参数
    QUrl url("https://aip.baidubce.com/rest/2.0/ocr/v1/license_plate");

    QUrlQuery query;
    query.addQueryItem("access_token", accessToken);
    url.setQuery(query);

    // 组装请求
    QNetworkRequest req;
    req.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/x-www-form-urlencoded"));
    req.setUrl(url);
    req.setSslConfiguration(sslconfig);

    imgManager->post(req, postData);
}

void carplate::imgReply(QNetworkReply *reply)
{
    if(reply->error() != QNetworkReply::NoError){
        qDebug()<<reply->errorString();
        return;
    }

    const QByteArray replyData = reply->readAll();
    //qDebug()<<replyData;
    QJsonParseError jsonErr;
    QJsonDocument doc = QJsonDocument::fromJson(replyData,&jsonErr);
    if(jsonErr.error == QJsonParseError::NoError){
        QString carinfo;
        //取出最外层json
        QJsonObject obj = doc.object();
        if(obj.contains("words_result")){
            QJsonObject resultObj = obj.take("words_result").toObject();
            if(resultObj.contains("vertexes_location")){
                QJsonArray vertexesArray = resultObj.value("vertexes_location").toArray();
                if (vertexesArray.size() == 4) {
                    iscar = true;
                    QJsonObject topLeftObj = vertexesArray.at(0).toObject();
                    QJsonObject topRightObj = vertexesArray.at(1).toObject();
                    QJsonObject bottomRightObj = vertexesArray.at(2).toObject();
                    QJsonObject bottomLeftObj = vertexesArray.at(3).toObject();

                    int topLeftX = topLeftObj.value("x").toInt();
                    int topLeftY = topLeftObj.value("y").toInt();
                    int topRightX = topRightObj.value("x").toInt();
                    int topRightY = topRightObj.value("y").toInt();
                    int bottomRightX = bottomRightObj.value("x").toInt();
                    int bottomRightY = bottomRightObj.value("y").toInt();
                    int bottomLeftX = bottomLeftObj.value("x").toInt();
                    int bottomLeftY = bottomLeftObj.value("y").toInt();

                    cartop = std::min(topLeftY, topRightY);
                    carleft = std::min(topLeftX, bottomLeftX);
                    carwidth = std::max(topRightX, bottomRightX) - carleft;
                    carheight = std::max(bottomLeftY, bottomRightY) - cartop;
                    qDebug()<<cartop;
                    qDebug()<<carleft;
                    qDebug()<<carwidth;
                    qDebug()<<carheight;
                }
            }
            if(resultObj.contains("number")){
                //iscar = true;
                carnumber = resultObj.take("number").toString();
                carinfo.append("车牌号:").append(carnumber).append("--");
            }

            if(resultObj.contains("color")){
                //iscar = true;
                carcolor = resultObj.take("color").toString();
                carcolor = colorConversion(carcolor);
                carinfo.append(carcolor).append("\r\n");

                insertToSql();
            }
        }
    }else{
        qDebug()<<"JSON ERR:"<<jsonErr.errorString();
    }
    reply->deleteLater();
    //prePostData();
}

QString carplate::colorConversion(const QString &color)
{
    static QMap<QString, QString> colorMap {
        {"blue", "蓝牌"},
        {"yellow", "黄牌"},
        {"green", "绿牌"},
        {"white", "白牌"},
        {"black", "黑牌"},
        {"other", "其他牌"}
    };

    if (colorMap.contains(color)) {
        return colorMap.value(color);
    } else {
        return color;
    }
}

void carplate::lockdialog()
{
    eventLoop.exec();
}

void carplate::unlockdialog()
{
    eventLoop.exit();

}

void carplate::clearRectAndInfo()
{
    // 清除矩形
    carleft = 0;
    cartop = 0;
    carwidth = 0;
    carheight = 0;

    // 清除信息
    carnumber.clear();
    carcolor.clear();
    iscar = false;
    //leaveTime = QDateTime();
}


void carplate::on_button_close_clicked()
{
    if(finder != nullptr && camera != nullptr && childThread != nullptr){
        childThread->quit();
        //childThread->wait();
        camera->stop();

        delete childThread;
        childThread = nullptr;

        delete camera;
        camera = nullptr;
        delete finder;
        finder = nullptr;
        delete imageCapture;
        imageCapture = nullptr;
        delete refreshTime;
        refreshTime = nullptr;
        delete netTimer;
        netTimer = nullptr;
        delete tokenManager;
        tokenManager = nullptr;
        delete imgManager;
        imgManager = nullptr;
    }
    QPixmap pix(":/start.png");
    ui->label->setPixmap(pix);
}


void carplate::on_button_camerainfo_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}


void carplate::on_button_table_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);


    // 执行数据库查询
    QSqlQuery query;
    query.prepare("SELECT car_number, car_color, enter_time, exit_time FROM car_info");
    if (query.exec()) {
        ui->tableWidget_select->clearContents();
        ui->tableWidget_select->setRowCount(0);
        int row = 0;
        while (query.next()) {
            QString carNumber = query.value(0).toString();
            QString carColor = query.value(1).toString();
            QString enterTime = query.value(2).toDateTime().toString("yyyy-MM-dd hh:mm:ss");
            QString exitTime = query.value(3).toDateTime().toString("yyyy-MM-dd hh:mm:ss");


            // 插入新行
            ui->tableWidget_select->insertRow(row);

            // 插入单元格数据
            QTableWidgetItem *itemCarNumber = new QTableWidgetItem(carNumber);
            QTableWidgetItem *itemCarColor = new QTableWidgetItem(carColor);
            QTableWidgetItem *itemEnterTime = new QTableWidgetItem(enterTime);
            QTableWidgetItem *itemExitTime = new QTableWidgetItem(exitTime);
            //QTableWidgetItem *itemFee = new QTableWidgetItem(calculateFee(enterTime, exitTime));

            ui->tableWidget_select->setItem(row, 0, itemCarNumber);
            ui->tableWidget_select->setItem(row, 1, itemCarColor);
            ui->tableWidget_select->setItem(row, 2, itemEnterTime);
            ui->tableWidget_select->setItem(row, 3, itemExitTime);
            //ui->tableWidget_select->setItem(row, 4, itemFee);

            row++;
        }
    } else {
        qDebug() << "Error executing SQL query: ";
    }
}


void carplate::on_button_exit_clicked()
{
    unlockdialog();
}
