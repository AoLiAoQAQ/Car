#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QImage>
#include <QByteArray>
class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr);
public slots:
    void dowork(QImage img,QThread*childThread);


signals:
    void resultReady(QByteArray pd,QThread*childThread);
};

#endif // WORKER_H
