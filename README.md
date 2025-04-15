# CommunicationSystem

### 介绍
开发环境：阿里云、Ubuntu、Qt Creator、Vim、gcc、Visual Studio Code
关键字： C/S架构、TCP/IP协议、mysql、libevent、多线程

######  项目表述/ 功能：
1. 聊天软件支持用户注册、添加好友、创建或添加群、上线提醒等功能，同时也支持用户间文件的传输；
2. 界面采用Qt，实现多窗口控制多进程任务，用户可以同时与多人聊天、传输文件；
3. 用户的主要数据存放在mysql数据库中，当用户上线时，系统从数据库中读取必要
信息存放在哈希表中，方便读取和使用，提高工作效率；
4. 程序采用libevent事件库用来处理用户之间通信传输等多任务高并发的问题，并
通过多线程来提升聊天软件处理事件的速度，提高用户的使用舒适度

