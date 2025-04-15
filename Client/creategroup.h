#ifndef CREATEGROUP_H
#define CREATEGROUP_H

#include <QWidget>
#include "chatinterface.h"

namespace Ui {
class CreateGroup;
}

class CreateGroup : public QWidget
{
    Q_OBJECT

public:
    explicit CreateGroup(ChatInterface *,const QString &);
    ~CreateGroup();

private slots:
    void on_pushButton_no_clicked();

    void on_pushButton_yes_clicked();

private:
    Ui::CreateGroup *ui;
    ChatInterface *chatinterface_;
    QString owner_;	//群主用户名
};

#endif // CREATEGROUP_H
