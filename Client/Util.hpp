#ifndef UTIL_HPP
#define UTIL_HPP

#include <QPixmap>
#include <QPainter>
#include <string>
#include <QJsonDocument>

#define PROTOCOL_SEP '\3'   //自定义协议分割字符

class Util
{
public:
    static QPixmap createCircularPixmap(const QPixmap &src) {
        // 获取图像的尺寸
        int size = qMin(src.width(), src.height());
        QPixmap circularPixmap(size, size);

        // 设置透明背景
        circularPixmap.fill(Qt::transparent);

        // 创建QPainter
        QPainter painter(&circularPixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        // 创建一个圆形遮罩
        QPainterPath path;
        path.addEllipse(0, 0, size, size);
        painter.setClipPath(path);

        // 绘制图像
        painter.drawPixmap(0, 0, size, size, src);

        return circularPixmap;
    }

    static size_t Split(const QString &src, const QString &sep, QStringList *arry)
       {
           int offset = 0;
           while (offset < src.size())
           {
               ssize_t pos = src.indexOf(sep, offset);
               // 没找到分割字符串
               if (pos == -1)
               {
                   // 直接把剩余部分当作一个字串，放入arry中
                   arry->push_back(src.mid(offset));
                   return arry->size();
               }

               // 当前字串为空
               if (offset == pos)
               {
                   offset = pos + sep.size();
                   continue;
               }

               // 找到了分割字符串
               arry->push_back(src.mid(offset, pos - offset));
               offset = pos + sep.size();
           }

           return arry->size();
       }

    static void Encode(const QString &packet, QByteArray *in)
    {
        //获取报文的长度
        size_t size = packet.size();
        //将其转换为字符串
        QString size_str = QString::number(size);
        //封装报头
        in->insert(0,size_str);
        in->append(PROTOCOL_SEP);
        in->append(packet);
    }

};

#endif // UTIL_HPP
