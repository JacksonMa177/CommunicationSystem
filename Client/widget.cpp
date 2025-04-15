#include "widget.h"
#include "ui_widget.h"
#include "Util.hpp"
#include "chatinterface.h"
#include <QTcpSocket>
#include <QHostAddress>
#include <QDebug>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <string>

#define SERVER_IP "YOUR_CLOUD_SERVER_IP"  // Replace with your cloud server's public IP
#define SERVER_PORT 8888

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    //设置ICon和窗口标题
    this->setWindowIcon(QIcon(":/image/icon.png"));
    this->setWindowTitle("QQ");
    //不能更改窗体大小
    this->setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);

    //绘制登录界面的Logo
    PainterLogo();

        //创建TCP套接字
    socket_ = new QTcpSocket(this);

    //向服务器发起连接
    socket_->connectToHost(QHostAddress(SERVER_IP),SERVER_PORT);

    //绑定信号槽
    connect(socket_,&QTcpSocket::connected,this,&Widget::socket_connect_success);
    connect(socket_,&QTcpSocket::disconnected,this,&Widget::socket_connect_failed);
    connect(socket_,&QTcpSocket::readyRead,this,&Widget::socket_OnMessage);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::SendJsonString(const QJsonObject &v)
{
    QByteArray sendData;
    QByteArray ba = QJsonDocument(v).toJson();

    int size = ba.size();

    sendData.insert(0, (char *)&size, 4);
    sendData.append(ba);

    qDebug() <<"client send:" << sendData;
    socket_->write(sendData);

    socket_->flush();
}



void Widget::PainterLogo()
{
    // 加载图像
    QPixmap originalPixmap(":/image/qq.jpg");
    //设置图片大小
    originalPixmap =  originalPixmap.scaled(80,80);
    // 创建圆形图像
    QPixmap circularPixmap = Util::createCircularPixmap(originalPixmap);
    // 设置 QLabel 的 QPixmap
    ui->label_avatar->setPixmap(circularPixmap);

    // 调整 QLabel 尺寸
    ui->label_avatar->setFixedSize(circularPixmap.size());

    // 设置 QLabel 的窗口标志和样式
    ui->label_avatar->setWindowFlags(Qt::FramelessWindowHint);
    ui->label_avatar->setAttribute(Qt::WA_TranslucentBackground, true);
    ui->label_avatar->setAlignment(Qt::AlignCenter);

}

void Widget::HandleRegister(const QJsonObject &v)
{
     QString result = v["result"].toString();

     if(result == "user_exist"){
         QMessageBox::warning(this,"注册提示","用户已存在");
     }else if(result == "success"){
         QMessageBox::information(this,"注册提示","注册成功");
     }
}

void Widget::HandleLogin(const QJsonObject &v)
{
     QString result = v["result"].toString();

     qDebug() << v;

     if( result == "already_online"){
         //用户已经在线
         QMessageBox::warning(this,"登录提示","你已登录一个相同的用户名，请勿重复登陆");
         return ;
     }else if(result == "not_exist"){
         //用户不存在
         QMessageBox::warning(this,"登录提示","用户名不存在");
         return;
     }else if(result == "password_error"){
         //密码不正确
         QMessageBox::warning(this,"登录提示","密码不正确");
     }else if(result == "success"){
//         //密码正确，登录成功

         //跳转至聊天列表
         ChatInterface *chatinterface = new ChatInterface(socket_,v,ui->lineEdit_username->text());
         chatinterface->setAttribute(Qt::WA_DeleteOnClose);
         chatinterface->show();

         //断开信号槽，这两个信号的槽函数是登录界面处理的，我们需要在聊天列表处理
         disconnect(socket_, &QTcpSocket::disconnected, this, &Widget::socket_connect_failed);
         disconnect(socket_, &QTcpSocket::readyRead, this, &Widget::socket_OnMessage);
         //隐藏登录界面
         this->hide();
     }
}

void Widget::socket_connect_success()
{
    QMessageBox::information(this,"连接成功","连接服务器成功");
}

void Widget::socket_connect_failed()
{
    QMessageBox::warning(this,"连接断开","与服务器断开连接");
}

//注册按钮槽函数
void Widget::on_pushButton_register_clicked()
{
    //获取用户名和密码
    QString username = ui->lineEdit_username->text();
    QString password = ui->lineEdit_password->text();

    //构建Json对象
    QJsonObject value;
    value.insert("cmd","register");
    value.insert("username",username);
    value.insert("password",password);

    //将Json构建为QByteArray
//    QByteArray sendStr = QJsonDocument(value).toJson();

    //发送数据
    SendJsonString(value);
}

void Widget::socket_OnMessage()
{
    QByteArray ba;
    char buf[1024] = {0};
    int size, sum = 0;
    bool flag = true;

    socket_->read(buf, 4);
    memcpy(&size, buf, 4);

//    qDebug() << "get data len : " << size;

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

        qDebug() << "data : " << ba;
    //走到这里str就是一个完整的报文了
    //将字符串str转换为Json对象
    QJsonObject value = QJsonDocument::fromJson(ba).object();

    qDebug() << "client recv:" <<value;

    QString cmd = value["cmd"].toString();
    if(cmd == "register_reply"){
        return HandleRegister(value);
    }else if(cmd == "login_reply"){
        return HandleLogin(value);
    }
}

//登录按钮槽函数
void Widget::on_pushButton_login_clicked()
{
    //获取用户名和密码
    QString username = ui->lineEdit_username->text();
    QString password = ui->lineEdit_password->text();

    QJsonObject value;
    value.insert("cmd","login");
    value.insert("username",username);
    value.insert("password",password);

    SendJsonString(value);

}
