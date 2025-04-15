#include "creategroup.h"
#include "ui_creategroup.h"
#include <QJsonObject>

CreateGroup::CreateGroup(ChatInterface *chatinterface,const QString & owner):
    ui(new Ui::CreateGroup),
    chatinterface_(chatinterface),
    owner_(owner)
{
    ui->setupUi(this);
}

CreateGroup::~CreateGroup()
{
    delete ui;
}

//取消按钮槽函数
void CreateGroup::on_pushButton_no_clicked()
{
    this->close();
}

//确定按钮槽函数
void CreateGroup::on_pushButton_yes_clicked()
{
     //获取群名称
    const QString &groupname = ui->lineEdit_groupname->text();

    //构建Json
    QJsonObject value;
    value.insert("cmd","creategroup");
    value.insert("groupname",groupname);
    value.insert("owner",owner_);

    //发送给服务器
    chatinterface_->SendJsonString(value);
}
