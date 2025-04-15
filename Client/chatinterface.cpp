#include "chatinterface.h"
#include "ui_chatinterface.h"
#include "Util.hpp"
#include "privatechat.h"
#include "creategroup.h"
#include "joingroup.h"
#include "groupchat.h"
#include <addfriend.h>
#include <QTcpSocket>
#include <QJsonObject>
#include <QMenuBar>
#include <QJsonDocument>
#include <QMessageBox>

ChatInterface::ChatInterface(QTcpSocket *socket,const QJsonObject &v,QString username)
        :ui(new Ui::ChatInterface)
{
    ui->setupUi(this);
    this->username_ = username;
    this->socket_ = socket;
    this->setWindowIcon(QIcon(":/image/icon.png"));
    this->setWindowTitle(username);
    this->move(1350,75);

    //不能更改窗口大小
    this->setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);

    //初始化好友列表和群列表
    InitList(v);

    //绑定信号槽
    connect(socket_,&QTcpSocket::readyRead,this,&ChatInterface::HandleOnMessage);

}

ChatInterface::~ChatInterface()
{
    delete ui;
}

void ChatInterface::closeEvent(QCloseEvent *event)
{
    QJsonObject obj;
    obj.insert("cmd", "offline");
    obj.insert("username", username_);

    SendJsonString(obj);

    socket_->disconnect(SIGNAL(disconnected()));

    socket_->close();

    socket_->deleteLater();

}

void ChatInterface::SendJsonString(const QJsonObject &v)
{
//    //将Json对象转换为字符串
//    QByteArray packet = QJsonDocument(v).toJson();

//    //封装报头
//    QByteArray sendStr;
//    Util::Encode(packet,&sendStr);


//    //发送给服务
//    socket_->write(sendStr,sendStr.size());



    QByteArray sendData;
    QByteArray ba = QJsonDocument(v).toJson();

    int size = ba.size();

    sendData.insert(0, (char *)&size, 4);
    sendData.append(ba);


    qDebug() <<"client send:" << sendData;
    socket_->write(sendData);

    socket_->flush();
}


bool ChatInterface::Decode(QByteArray *str)
{
    int pos = str->indexOf(PROTOCOL_SEP);
    if(pos == -1){
        //数据不完整
        qDebug() << "未找到报头分隔符";
        return false;
    }

    //报头字段
    QString size_str = str->mid(0,pos);

    //将报头字段转换为整形
    int size = size_str.toInt();

    //移除报头字段
    str->remove(0,pos + 1); //这里加1是为了把'\3'也移除掉

    bool flag = true;
    while(flag){
        if(str->size() >= size){
            flag = false;
            break;
        }

        char buf[1024] = {0};
        socket_->read(buf,size - str->size());
        str->append(buf);
    }

    return true;
}

void ChatInterface::InitList(const QJsonObject &v)
{
    //获取用户名，好友列表和群列表
    this->friendList_ = v["friendlist"].toString();
    this->groupList_ = v["grouplist"].toString();
    qDebug() <<groupList_;

    //设置图标
    ui->tabWidget->setTabIcon(0,QIcon(":/image/icon.png"));
    ui->tabWidget->setTabIcon(1,QIcon(":/image/icon.png"));

    //好友列表和群列表初始化
    QStringList friendlist;
    Util::Split(this->friendList_,"|",&friendlist);
    QStringList grouplist;
    Util::Split(this->groupList_,"|",&grouplist);
    qDebug() <<grouplist;
    ui->listWidget_friend->addItems(friendlist);
    ui->listWidget_group->addItems(grouplist);

//    ui->listWidget_friend->setGridSize(QSize(100,60));
//    ui->listWidget_group->setGridSize(QSize(100,60));
}

void ChatInterface::HandleOnMessage()
{
    QByteArray ba;

    char buf[1024] = {0};
    int size, sum = 0;
    bool flag = true;

    socket_->read(buf, 4);
    memcpy(&size, buf, 4);


    while (flag)
    {
        memset(buf, 0, 1024);
        sum += socket_->read(buf, size - sum);
        if (sum >= size)
        {
            flag = false;
        }

        ba.append(buf);
    }

    //进行报头和有效载荷的分离
//    Decode(&str);

    //走到这里str就是一个完整的报文了
    //将字符串str转换为Json对象
    QJsonObject value = QJsonDocument::fromJson(ba).object();
    qDebug() << "client recv:" <<value;

    QString cmd = value["cmd"].toString();
    if(cmd == "online"){
        //好友上线提醒
        QString tmp = value["username"].toString() + "以上线";
        QMessageBox::information(this,"好友上线",tmp);
    }else if(cmd == "addfriend_reply"){
        //添加好友处理
        HandleAddFriend(value);
        return ;
    }else if(cmd == "be_addfriend"){
        //添加好友回复提醒
        QString tmp = value["friend"].toString() + "已添加你为好友";
        QMessageBox::information(this,"添加好友提示",tmp);

        //好友列表中新增item
        ui->listWidget_friend->addItem(value["friend"].toString());
    }else if(cmd == "private_reply"){
        //对方不在线
        QString tmp = value["friend"].toString() + "当前不在线";
        QMessageBox::information(this,"私聊提示",tmp);
    }else if(cmd == "private"){
        //私聊转发消息处理
        HandlePrivate(value);
    }else if(cmd == "creategroup_reply"){
        //创建群聊消息处理
        HandleCreatGroup(value);
    }else if(cmd == "joingroup_reply"){
        //加入群处理
        HandleJoinGroup(value);
    }else if(cmd == "groupchat_reply"){
        //群聊转发消息处理
        HandleGroupChat(value);
    }else if(cmd == "groupmember_reply"){
        //获取群成员,
        QStringList member;
        Util::Split(value["member"].toString(),"|",&member);
        //发送这个信号，把群成员传送给GroupChat窗口
        emit SignalGroupMember(member);
    }else if (cmd == "new_member_join"){
        QString name = value["username"].toString();
        QMessageBox::information(this,"群聊提示",name+"加入群聊：" + value["groupname"].toString());

    }else if(cmd == "file_reply"){
        //发送文件属性回复
        HandleSendFile(value);
    }else if(cmd == "file_name"){
        //接收文件属性处理
        QString str = value["filename"].toString();
        int idx = str.lastIndexOf('/');
        file.filename = str.right(str.length() - idx - 1);
        QString tmp = value["filelength"].toString();
        file.length = std::stoi(tmp.toStdString());
        file.fromuser = value["fromuser"].toString();
        file.file = new QFile(file.filename);
        file.file->open(QIODevice::WriteOnly);
    }else if (cmd == "file_transfer")
    {
        QString text = value["text"].toString();
        QByteArray writeData;
        writeData.append(text);
        file.file->write(writeData);
    }
    else if (cmd == "file_end")
    {
        file.file->close();
        delete file.file;

        QString str = QString("%1给你发送了一个文件").arg(file.fromuser);
        QMessageBox::information(this, "文件传输", str);
    }else if(cmd == "friend_offline"){
        QString str = QString("%1下线").arg(value.value("username").toString());

        QMessageBox::information(this, "下线提醒", str);
    }
}

void ChatInterface::HandleAddFriend(const QJsonObject &v)
{
     const QString &result = v["result"].toString();
//     qDebug() <<result;
     if(result == "not_exist"){
         //好友不存在
         QMessageBox::warning(this,"添加好友提示","当前用户名不存在");
     }else if(result == "already_friend"){
        //已经是好友了
         QString tmp = v["friend"].toString() + "已经是你的好友了";
         QMessageBox::warning(this,"添加好友提示",tmp);
     }else if(result == "success"){
         //添加好友成功
         QString tmp = "添加好友 "+v["friend"].toString() + "成功";
         QMessageBox::information(this,"添加好友提示",tmp);

         //好友列表中新增item
         ui->listWidget_friend->addItem(v["friend"].toString());
     }

}

void ChatInterface::HandleCreatGroup(const QJsonObject &v)
{
     //获取result
     const QString &result = v["result"].toString();
     if(result == "exist"){
         //群聊已经存在
         const QString &tmp = v["groupname"].toString() + "已经存在";
         QMessageBox::warning(this,"创建群聊提示",tmp);
     }else if(result == "success"){
         //群聊创建成功，显示进群聊列表
         const QString &tmp = v["groupname"].toString() + "创建成功";
         QMessageBox::information(this,"创建群聊提示",tmp);

         //添加至群聊列表中
         ui->listWidget_group->addItem(v["groupname"].toString());
     }
}

void ChatInterface::HandleJoinGroup(const QJsonObject &v)
{
     //获取result
     const QString &result = v["result"].toString();
     if(result == "not_exist") {
         //群聊不存在
         const QString &tmp = "您要加入的群聊： " + v["groupname"].toString() + " 不存在";
         QMessageBox::warning(this,"加入群聊提示",tmp);
     }else if(result == "already"){
         //已经在群里了
         const QString &tmp ="你已经在 " + v["groupname"].toString() + " 里了";
         QMessageBox::warning(this,"加入群聊提示",tmp);
     }else if(result == "success"){
         //加入群聊成功
         const QString &tmp ="加入群聊 " + v["groupname"].toString() + " 成功";
         QMessageBox::warning(this,"加入群聊提示",tmp);

         //将群聊名显示在群聊列表上
         ui->listWidget_group->addItem(v["groupname"].toString());

     }
}

//私聊窗口的创建
void ChatInterface::on_listWidget_friend_itemDoubleClicked(QListWidgetItem *item)
{
    //获取好友姓名来充当标题
    QString title = item->text();

    auto it = privateWidgets_.find(title);
    if(it != privateWidgets_.end()){
        //窗口已经打开
        //让这个窗口显示在最上面
        it->second->activateWindow();
        return;
    }

    //没找到就插入进哈希表中
    PrivateChat *privatechat = new PrivateChat(title,username_,this);
    privatechat->setAttribute(Qt::WA_DeleteOnClose);
    privateWidgets_[title] = privatechat;
    privatechat->show();
}

void ChatInterface::HandlePrivate(const QJsonObject &v)
{
    //先看看PrivateCHat窗口存不存在，不存在就创建一个并显示
    auto it = this->privateWidgets_.find(v["fromfriend"].toString());
    if(it == privateWidgets_.end()){
        PrivateChat *privatechat = new PrivateChat(v["fromfriend"].toString(),username_,this);
        privatechat->setAttribute(Qt::WA_DeleteOnClose);
        privateWidgets_[v["fromfriend"].toString()] = privatechat;
        privatechat->show();
    }
    //发送这个信号，把接收到的Json传送给privatechat窗口
    emit signalPrivate(v);
}

//创建群聊按钮槽函数
void ChatInterface::on_pushButton_creategroup_clicked()
{
    CreateGroup *creategroup = new CreateGroup(this,username_);
    creategroup->setAttribute(Qt::WA_DeleteOnClose);
    creategroup->show();
}

//加入群聊按钮槽函数
void ChatInterface::on_pushButton_joingroup_clicked()
{
     JoinGroup *joingroup = new JoinGroup(this,username_);
    joingroup->setAttribute(Qt::WA_DeleteOnClose);
    joingroup->show();
}

//群聊列表item双击槽函数
void ChatInterface::on_listWidget_group_itemDoubleClicked(QListWidgetItem *item)
{
     //获取item的text充当群聊标题
     const QString title = item->text();

     auto it = groupWidgets_.find(title);
     if(it != groupWidgets_.end()){
         //窗口已经打开了
         it->second->activateWindow();
         return;
     }

     //构建JSon,这个Json是为了获取群聊的群成员
     QJsonObject value;
     value.insert("cmd","groupmember");
     value.insert("groupname",item->text());
     //发送给服务器
     SendJsonString(value);

     //窗口没打开
     GroupChat *groupchat = new GroupChat(this,title,username_);
     groupchat->setAttribute(Qt::WA_DeleteOnClose);
     groupchat->show();

     //将当前窗口插入哈希表中
     groupWidgets_.insert(std::make_pair(title,groupchat));
}

void ChatInterface::HandleGroupChat(const QJsonObject &v)
{
     //获取群名称，是哪一个群有新消息了
     const QString &groupname = v["groupname"].toString();

     auto it = groupWidgets_.find(groupname);
     if(it != groupWidgets_.end()){
         //窗体存在
         emit SignalGroupChat(v);
         it->second->activateWindow();
         return;
     }

     //构建JSon,这个Json是为了获取群聊的群成员
     QJsonObject value;
     value.insert("cmd","groupmember");
     value.insert("groupname",groupname);
     //发送给服务器
     SendJsonString(value);

     //窗体不存在
     GroupChat *groupchat = new GroupChat(this,v["groupname"].toString(),username_);
     groupchat->setAttribute(Qt::WA_DeleteOnClose);
     groupchat->show();

     //将当前窗口插入哈希表中
     groupWidgets_.insert(std::make_pair(v["groupname"].toString(),groupchat));

     emit SignalGroupChat(v);

}

void ChatInterface::HandleSendFile(const QJsonObject &v)
{
    if(v["result"].toString() == "offline"){
        //好友不在线
        emit SignalFriendOffine(v);
    }else if(v["result"].toString() == "online"){
        //好友在线
        qDebug() << "----------";
        emit SignalFriendOnline(v);
    }
}

void ChatInterface::on_pushButton_addfrined_clicked()
{
     AddFriend *addfriend = new AddFriend(this,username_);
     addfriend->setAttribute(Qt::WA_DeleteOnClose);
     addfriend->show();
}
