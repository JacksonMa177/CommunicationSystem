#include "sendfile.h"
#include "Util.hpp"
#include <QHostAddress>
#include <QFile>
#include <QJsonObject>
#include <unistd.h>

SendFile::SendFile(const QString &filename,const QString &friendname) :
    filename_(filename),
    friendname_(friendname)
{

}


void SendFile::working(){
    //创建socket对象
        sendSocket = new QTcpSocket;

        sendSocket->connectToHost(QHostAddress("110.40.209.26"), 8888);

        if (!sendSocket->waitForConnected(5000))
        {
            //连接服务器失败
            emit connect_timeout();

            return;
        }

        //打开文件
        QFile *m_file = new QFile(filename_);
        m_file->open(QIODevice::ReadOnly);


        QByteArray readData;

        while (true)
        {
            readData.clear();

            readData = m_file->read(512);

            if (readData.isEmpty())
            {
                //文件读取完毕
                QJsonObject sendObj;
                sendObj.insert("cmd", "file");
                sendObj.insert("step", "3");
                sendObj.insert("friendname", friendname_);

                SendJsonString(sendObj);

                break;
            }

            QJsonObject sendObj;
            sendObj.insert("cmd", "file");
            sendObj.insert("step", "2");
            sendObj.insert("friendname", friendname_);
            sendObj.insert("text", QString(readData));

            SendJsonString(sendObj);

            usleep(100000);
        }

        m_file->close();
        delete m_file;

        sleep(1);
        sendSocket->close();
        delete sendSocket;

        //通知主线程
        qDebug() << "----- 给主线程发送信号";
        emit thread_send_finish();
}


void SendFile::SendJsonString(const QJsonObject &v)
{
    QByteArray sendData;
    QByteArray ba = QJsonDocument(v).toJson();

    int size = ba.size();

    sendData.insert(0, (char *)&size, 4);
    sendData.append(ba);

    qDebug() << sendData << "\n\n";
    sendSocket->write(sendData);

    sendSocket->flush();
}
