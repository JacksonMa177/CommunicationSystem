#include "joingroup.h"
#include "ui_joingroup.h"
#include <QJsonObject>

JoinGroup::JoinGroup(ChatInterface *chatinterface, const QString &username) :
    ui(new Ui::JoinGroup),
    chatinterface_(chatinterface),
    username_(username)
{
    ui->setupUi(this);
}


JoinGroup::~JoinGroup()
{
    delete ui;
}

void JoinGroup::on_pushButton_no_clicked()
{
    this->close();
}

void JoinGroup::on_pushButton_yes_clicked()
{
    //获取群聊名称
    const QString &groupname = ui->lineEdit_groupname->text();

    //构建Json
    QJsonObject value;
    value.insert("cmd","joingroup");
    value.insert("username",this->username_);
    value.insert("groupname",groupname);

    //发送给服务器
    chatinterface_->SendJsonString(value);
}
