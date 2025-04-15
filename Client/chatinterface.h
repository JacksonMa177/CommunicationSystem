#ifndef CHATINTERFACE_H
#define CHATINTERFACE_H

//#include "privatechat.h"

#include <QWidget>
#include <QTcpSocket>
#include <QListWidgetItem>
#include <unordered_map>
#include <QFile>

class PrivateChat;
class GroupChat;

struct FileInfo
{
    QString filename;
    QString fromuser;
    int length;
    QFile *file;
};


namespace Ui {
class ChatInterface;
}

class ChatInterface : public QWidget
{
    Q_OBJECT

public:
    explicit ChatInterface(QTcpSocket *,const QJsonObject &,QString);
    ~ChatInterface();
    void closeEvent(QCloseEvent *event) ;

public:
    void SendJsonString(const QJsonObject &);
private:
    void InitList(const QJsonObject &);
    bool Decode(QByteArray *str);
private slots:
    //可读事件回调
    void HandleOnMessage();
    //添加好友处理回调
    void HandleAddFriend(const QJsonObject &);
    //创建群聊处理回调
    void HandleCreatGroup(const QJsonObject &);
    //加入群聊处理回调
    void HandleJoinGroup(const QJsonObject &);
    //添加好友按钮槽函数
    void on_pushButton_addfrined_clicked();
    //好友列表双击槽函数
    void on_listWidget_friend_itemDoubleClicked(QListWidgetItem *item);
    //私聊处理槽函数
    void HandlePrivate(const QJsonObject &);
    //创建群聊按钮槽函数
    void on_pushButton_creategroup_clicked();
    //加入群聊按钮槽函数
    void on_pushButton_joingroup_clicked();
    //群列表双击槽函数
    void on_listWidget_group_itemDoubleClicked(QListWidgetItem *item);
    //群聊处理槽函数
    void HandleGroupChat(const QJsonObject &);
    //发送文件回复槽函数
    void HandleSendFile(const QJsonObject &);

signals:
    //发送这个信号,把从服务器接收到的私聊转发的数据传送给privateChat窗体
    void signalPrivate(const QJsonObject &);
    /*这两个信号功能重复了*/
    //发送这个信号，把从服务器接收到的加入群聊成功回复中的群成员姓名的list传送给GroupChat窗口
    void signalGroup(const QStringList &);
    //发送这个信号，把从服务器接收到的groupmember_reply，群成员的list传送给groupchat窗口
    void SignalGroupMember(const QStringList &);
    //发送这个信号,把从服务器接收到的群聊转发的数据传送给groupChat窗体
    void SignalGroupChat(const QJsonObject &);

    void SignalFriendOffine(const QJsonObject &);
    void SignalFriendOnline(const QJsonObject &);
private:
    Ui::ChatInterface *ui;
    QTcpSocket *socket_;
    QString friendList_;
    QString groupList_;
    FileInfo file;
public:
    QString username_;	//共有接口，方便私聊使用

    //维护已经打开的私聊窗口
    std::unordered_map<QString,PrivateChat*> privateWidgets_;
    //维护已经打开的群聊窗口
    std::unordered_map<QString,GroupChat*> groupWidgets_;
};

#endif // CHATINTERFACE_H
