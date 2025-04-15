#include "privatechat.h"
#include "ui_privatechat.h"
#include "Util.hpp"
#include "sendfile.h"
#include <QJsonObject>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include <QJsonDocument>
#include <QHostAddress>
#include <unistd.h>


PrivateChat::PrivateChat(QString title,QString username,ChatInterface *chatinterface) :
    ui(new Ui::PrivateChat),
    chatinterface_(chatinterface),
    title_(title),
    username_(username)
{
    ui->setupUi(this);
    //设置标题
    ui->label_title->setText(title_); 

    //设置窗口标题
    this->setWindowTitle("私聊");
    this->setWindowIcon(QIcon(":/image/icon.png"));

    //绑定信号槽,chatinterface的privatechat信号，绑定槽函数HandlePrivate
    connect(chatinterface_,&ChatInterface::signalPrivate,this,&PrivateChat::HandlePrivate);


    connect(chatinterface,&ChatInterface::SignalFriendOffine,this,[=](){
        QMessageBox::information(this,"发送文件提示","好友不在线，发送文件失败");
    });

    connect(chatinterface,&ChatInterface::SignalFriendOnline,this,&PrivateChat::HandleSendFile);
}

PrivateChat::~PrivateChat()
{
    delete ui;
}

//发送按钮槽函数
void PrivateChat::on_pushButton_send_clicked()
{
    //获取textEdit的文本
    QString text = ui->textEdit->toPlainText();

    //组织Json
    QJsonObject value;
    value.insert("cmd","private");
    value.insert("username",chatinterface_->username_);
    value.insert("tofriend",title_);
    value.insert("text",text);

    //将消息显示在listWidget上
    ui->content->addItem("我："+text);

    //将发送框文本清空
    ui->textEdit->setText("");

    //发送给服务器
    chatinterface_->SendJsonString(value);
}

//关闭按钮槽函数
void PrivateChat::on_pushButton_cancel_clicked()
{
    //当前窗口从哈希表中移除
    auto it = chatinterface_->privateWidgets_.find(title_);
    if(it != chatinterface_->privateWidgets_.end()){
        chatinterface_->privateWidgets_.erase(title_);
    }

    this->close();
}


void PrivateChat::HandlePrivate(const QJsonObject &v)
{
      ui->content->addItem(title_+": " + v["text"].toString());
}

void PrivateChat::closeEvent(QCloseEvent *event)
{
    (void)event;

    //当前窗口从哈希表中移除
    auto it = chatinterface_->privateWidgets_.find(title_);
    if(it != chatinterface_->privateWidgets_.end()){
        chatinterface_->privateWidgets_.erase(title_);
    }

    this->close();
}

void PrivateChat::on_pushButton_file_clicked()
{
     //弹出一个文件框，选择要发送的文件
     filename_ = QFileDialog::getOpenFileName(this,"发送文件");
     if(!filename_.isEmpty()){
         QFile file(filename_);

//         //将文件名分离出来
//         auto pos = name.lastIndexOf('/');
//         filename_ = name.mid(pos + 1);
//         qDebug() << name;

         //构建JSon
         QJsonObject value;
         value.insert("cmd","file");
         value.insert("step","1");
         value.insert("filename",filename_);
         value.insert("filelength",QString::number(file.size()));
         value.insert("friendname",title_);
         value.insert("username",username_);

         //发送给服务器
         chatinterface_->SendJsonString(value);
     }
}


void PrivateChat::HandleSendFile(const QJsonObject &v)
{
    //启动线程，开始传输文件
    //创建线程对象
    QThread *subThread = new QThread;

    //创建发送文件对象
    SendFile *mySendFile = new SendFile(filename_, title_);

    mySendFile->moveToThread(subThread);

    //启动线程
    subThread->start();

    //通过信号启动线程工作函数
    connect(this, &PrivateChat::signal_start_send_file, mySendFile, &SendFile::working);
    emit signal_start_send_file();

    //文件传输完成
    connect(mySendFile, &SendFile::thread_send_finish, this, [=]()
    {
        subThread->quit();
        subThread->wait();
        subThread->deleteLater();
        mySendFile->deleteLater();

        QMessageBox::information(this, "文件传输", "文件发送完成");
    });

    connect(mySendFile, &SendFile::connect_timeout, this,[=](){
        QMessageBox::warning(this, "文件传输", "子线程连接服务器超时");
    });

}
