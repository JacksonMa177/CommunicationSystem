#ifndef PRIVATECHAT_H
#define PRIVATECHAT_H

#include <QWidget>
#include "chatinterface.h"

namespace Ui {
class PrivateChat;
}

class PrivateChat : public QWidget
{
    Q_OBJECT

public:
    explicit PrivateChat(QString title,QString username,ChatInterface *);
    ~PrivateChat();
private:
    void SendJsonString(QTcpSocket *socket ,const QJsonObject &v);
private slots:
    void on_pushButton_send_clicked();
    void on_pushButton_cancel_clicked();
    //转发私聊处理槽函数
    void HandlePrivate(const QJsonObject &);

    void on_pushButton_file_clicked();
    void HandleSendFile(const QJsonObject &);

signals:
    //发送这个信号来进行发送读取文件发送的操作
    void signal_start_send_file();
public:
    //重写窗口关闭事件，把当前窗口从privateWidgets_哈希表中移除
    void closeEvent(QCloseEvent *event);

private:
    Ui::PrivateChat *ui;
    ChatInterface *chatinterface_;
    QString title_;//好友用户名
    QString username_;	//自己用户户名
    QString filename_; //发送文件的文件名
};

#endif // PRIVATECHAT_H
