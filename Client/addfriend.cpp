#include "addfriend.h"
#include "ui_addfriend.h"
#include <QJsonObject>

AddFriend::AddFriend(ChatInterface *chatinterface,QString username)
    :ui(new Ui::AddFriend),
    Chatinterface_(chatinterface),
    username_(username)
{
    ui->setupUi(this);
}

AddFriend::~AddFriend()
{
    delete ui;
}

//确定按钮
void AddFriend::on_pushButton_yes_clicked()
{
     //获取用户名
     QString friendname = ui->lineEdit_username->text();

     //构建Json
     QJsonObject value;
     value.insert("cmd","addfriend");
     value.insert("username",username_);
     value.insert("friend",friendname);

     //发送给服务器
     Chatinterface_->SendJsonString(value);

}

void AddFriend::on_pushButton_no_clicked()
{
    this->close();
}
