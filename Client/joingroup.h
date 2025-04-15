#ifndef JOINGROUP_H
#define JOINGROUP_H

#include <QWidget>
#include "chatinterface.h"

namespace Ui {
class JoinGroup;
}

class JoinGroup : public QWidget
{
    Q_OBJECT

public:
    explicit JoinGroup(ChatInterface*,const QString &);
    ~JoinGroup();

private slots:
    void on_pushButton_no_clicked();

    void on_pushButton_yes_clicked();

private:
    Ui::JoinGroup *ui;
    ChatInterface *chatinterface_;
    QString username_;//哪个用户要加入群聊
};

#endif // JOINGROUP_H
