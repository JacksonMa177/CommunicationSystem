#ifndef ADDFRIEND_H
#define ADDFRIEND_H

#include <QWidget>
#include "chatinterface.h"

namespace Ui {
class AddFriend;
}

class AddFriend : public QWidget
{
    Q_OBJECT

public:
    explicit AddFriend(ChatInterface *chatinterface,QString username);
    ~AddFriend();

private slots:
    void on_pushButton_yes_clicked();

    void on_pushButton_no_clicked();

private:
    Ui::AddFriend *ui;
    ChatInterface *Chatinterface_;	//通过这个指针来进行数据发送
    QString username_;
};

#endif // ADDFRIEND_H
