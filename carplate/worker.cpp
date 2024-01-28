#include "worker.h"
#include <QBuffer>
#include <QUrl>
Worker::Worker(QObject *parent) : QObject(parent)
{

}

void Worker::dowork(QImage img,QThread*childThread)
{
    // 拼接base64编码
    QByteArray ba;
    QBuffer buff(&ba);
    img.save(&buff, "png");
    QString b64str = ba.toBase64();
    QString urlEncodedStr = QUrl::toPercentEncoding(b64str);

    // 构建请求体数据
    QByteArray postData;
    postData.append("image=");
    postData.append(urlEncodedStr);

    emit resultReady(postData,childThread);
}
