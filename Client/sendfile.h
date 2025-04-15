#ifndef SENDFILE_H
#define SENDFILE_H

#include <QObject>
#include <QTcpSocket>


class SendFile : public QObject
{
    Q_OBJECT
public:
    explicit SendFile(const QString &filename,const QString &friendname_);
public:
    void working();
    void SendJsonString(const QJsonObject &v);
signals:
    //当发送文件完成时，发送这个信号来通知主线程，来做清理工作
    void thread_send_finish();
    void connect_timeout();
private:
    QTcpSocket *sendSocket;
    QString filename_;//文件名
    QString friendname_;//好友姓名
};

#endif // SENDFILE_H
