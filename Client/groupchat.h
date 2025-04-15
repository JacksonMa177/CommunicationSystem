#ifndef GROUPCHAT_H
#define GROUPCHAT_H

#include <QWidget>
#include "chatinterface.h"

namespace Ui {
class GroupChat;
}

class GroupChat : public QWidget
{
    Q_OBJECT

public:
    explicit GroupChat(ChatInterface *,const QString title,QString username);
    ~GroupChat();

private slots:
    //关闭按钮的点击槽函数
    void on_pushButton_cancel_clicked();
    //获取群成员姓名列表
    void HandleGetGroupMember(const QStringList &);
    //发送按钮槽函数
    void on_pushButton_send_clicked();
    //由Chatinterface发送自定义信号触发，获取服务器转发的群聊消息，并显示在content上
    void HandleGroupChat(const QJsonObject &);
private:
    //重写关闭事件，从哈希表中移除
    void closeEvent(QCloseEvent *event);

private:
    Ui::GroupChat *ui;
    ChatInterface *chatinterface_;
    QString title_;	//群聊的标题
    QStringList memberList_; //保存群成员姓名
    QString username_; //当前群聊窗口是哪个用户打开的
};

#endif // GROUPCHAT_H
