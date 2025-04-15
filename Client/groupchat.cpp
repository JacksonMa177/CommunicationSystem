#include "groupchat.h"
#include "ui_groupchat.h"
#include <QJsonObject>

GroupChat::GroupChat(ChatInterface *chatinterface,const QString title,QString username) :
    ui(new Ui::GroupChat),
    chatinterface_(chatinterface),
    title_(title),
    username_(username)
{
    ui->setupUi(this);
    //不能更改窗口大小
    this->setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);

    //设置标题
    ui->label_title->setText(title_);
    //设置窗口标题
    this->setWindowTitle("群聊");
    this->setWindowIcon(QIcon(":/image/icon.png"));

    //绑定信号槽，获取群成员姓名QStringlist
    connect(chatinterface_,&ChatInterface::signalGroup,this,&GroupChat::HandleGetGroupMember);
    connect(chatinterface_,&ChatInterface::SignalGroupMember,this,&GroupChat::HandleGetGroupMember);
    //显示服务器转发的群聊信息
    connect(chatinterface_,&ChatInterface::SignalGroupChat,this,&GroupChat::HandleGroupChat);
}

GroupChat::~GroupChat()
{
    delete ui;
}

//关闭按钮槽函数
void GroupChat::on_pushButton_cancel_clicked()
{
    //从哈希表中移除
    auto it = chatinterface_->groupWidgets_.find(title_);
    if(it != chatinterface_->groupWidgets_.end()){
        chatinterface_->groupWidgets_.erase(title_);
    }

    //关闭界面
    this->close();
}

void GroupChat::HandleGetGroupMember(const QStringList &memberList)
{
    //将获取到的群成员姓名赋值给当前窗体维护
    this->memberList_ = memberList;

    //先将群成员清空
    ui->listWidget_menber->clear();

    //将群成员姓名显示到群聊窗体上
    ui->listWidget_menber->addItems(memberList_);
}

void GroupChat::closeEvent(QCloseEvent *event)
{
    (void) event;

     //从哈希表中移除
    auto it = chatinterface_->groupWidgets_.find(title_);
    if(it != chatinterface_->groupWidgets_.end()){
        chatinterface_->groupWidgets_.erase(title_);
    }

    //关闭界面
    this->close();
}

void GroupChat::on_pushButton_send_clicked()
{
    //获取输入框的文本
    QString text = ui->textEdit->toPlainText();

    //构建JSon
    QJsonObject value;
    value.insert("cmd","groupchat");
    value.insert("groupname",title_);
    value.insert("username",username_);
    value.insert("text",text);

    //将要发送的信息显示在content上
    ui->content->addItem("我: " + text);

    //将输入框清空
    ui->textEdit->setText("");

    //发送给服务器
    chatinterface_->SendJsonString(value);
}

void GroupChat::HandleGroupChat(const QJsonObject &v)
{
     //将消息添加到content上显示
     QString from = v["from"].toString();
     QString text = v["text"].toString();

     ui->content->addItem(from + ": " + text);
}
