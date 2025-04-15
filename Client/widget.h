#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTcpSocket>
#include <QJsonObject>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
private:
    //封装报头
    void Encode(const QString &,QByteArray *);
    //将一个Json转换为字符串，封装报头，并发送
    void SendJsonString(const QJsonObject &);
    bool Decode(QByteArray *str);
private:
    void PainterLogo();
    void HandleRegister(const QJsonObject &);
    void HandleLogin(const QJsonObject &);
public slots:
    //connected信号触发 槽函数
    void socket_connect_success();
    //disconnected信号触发 槽函数
    void socket_connect_failed();
private slots:
    //注册按钮的槽函数
    void on_pushButton_register_clicked();
    //缓冲区有数据槽函数
    void socket_OnMessage();

    void on_pushButton_login_clicked();

private:
    Ui::Widget *ui;
    QTcpSocket *socket_;
};
#endif // WIDGET_H
