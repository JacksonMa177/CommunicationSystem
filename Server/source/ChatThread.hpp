#pragma once

#include "ChatInfo.hpp"
#include "ChatDataBase.hpp"

#include <event.h>
#include <json/json.h>

#include <thread>

#define PROTOCOL_SEP '\3' // 自定义协议分割字段
class ChatThread
{
private:
    std::thread *thread_;     // 线程
    std::thread::id Id_;      // 线程ID
    struct event_base *base_; // 子线程的事件集合

    // 由于子线程需要进行事件处理，所以需要涉及到数据库和两个数据结构的操作
    ChatInfo *chatInfo_;
    ChatDataBase *database_;

private:
    // 线程的入口函数
    static void Worker(ChatThread *chatThread)
    {
        chatThread->run();
    }

    // 定时器事件的回调函数 timer_cb 会在定时器触发时被调用
    static void timer_cb(evutil_socket_t fd, short what, void *arg)
    {
        // ChatThread *chatThread = static_cast<ChatThread *>(arg);
        // lg(Info,"%d is listening ...",chatThread->get_id()); //for debug
    }

    // 由于线程的入口函数是一个静态函数，他不能调用非静态的成员，所以我们把线程的要执行的任务放到这个run函数中来完成
    void run()
    {
        // 由于子线程的基础事件集合中没有监听事件，所以我们给这个集合中加入一个定时器事件
        lg(Info, "Thread-%d", std::this_thread::get_id()); // 打印日志

        // 创建一个事件指针
        struct event *timeout_event;

        // 设置定时时间
        struct timeval tv = {3, 0};

        // 初始化定时器事件
        timeout_event = event_new(base_, -1, EV_PERSIST, timer_cb, this);

        // 设置定时器,使用 event_add 将事件添加到 event_base 中
        event_add(timeout_event, &tv);

        // 开始事件循环
        event_base_dispatch(base_);

        // 释放资源
        event_base_free(base_);
    }

    // 向客户端发送Json字符串
    void SendJsonString(struct bufferevent *bev, const Json::Value &value)
    {

        std::string s = Json::FastWriter().write(value);
        int len = s.size();
        char buf[1024] = {0};
        memcpy(buf, &len, 4);
        memcpy(buf + 4, s.c_str(), len);
        if (bufferevent_write(bev, buf, len + 4) == -1)
        {
            std::cout << "bufferevent_write error" << std::endl;
        }
    }

    // 注册模块
    void HandleRegister(struct bufferevent *bev, const Json::Value &v)
    {

        const std::string &username = v["username"].asString();

        // 判断用户是否存在
        bool ret = database_->UserExist(username);
        if (ret)
        {
            // 用户已经存在 返回用户已存在Json字符串
            Json::Value value;
            value["cmd"] = "register_reply";
            value["result"] = "user_exist";

            SendJsonString(bev, value);
        }
        else
        {
            // 用户不存在 将用户信息插入数据库，返回用户不存在Json字符串
            database_->InsertChatUser(v);

            Json::Value value;
            value["cmd"] = "register_reply";
            value["result"] = "success";

            SendJsonString(bev, value);
        }
    }

    // 登录模块
    void HandleLogin(struct bufferevent *bev, const Json::Value &v)
    {
        // 判断用户信息是否存在于数据库
        if (database_->UserExist(v["username"].asString()) == false)
        {
            // 用户不存在
            Json::Value value;
            value["cmd"] = "login_reply";
            value["result"] = "not_exist";

            SendJsonString(bev, value);
            return;
        }

        // 用户存在，判断密码是否正确
        if (database_->AuthenticationPassword(v) == false)
        {
            // 密码不正确
            Json::Value value;
            value["cmd"] = "login_reply";
            value["result"] = "password_error";

            SendJsonString(bev, value);
            return;
        }

        if(chatInfo_->IsOnlineUser(v["username"].asString()) != nullptr){
            //用户已经在线了,返回客户端
            Json::Value value;
            value["cmd"] = "login_reply";
            value["result"] = "already_online";

            SendJsonString(bev, value);
            return;
        }

        // 用户存在，且密码正确,返回登陆成功Json
        Json::Value value;
        value["cmd"] = "login_reply";
        value["result"] = "success";
        // 获取好友列表和群列表信息
        std::string friendlist;
        std::string grouplist;
        database_->FriendListAndGroupList(v["username"].asString(), &friendlist, &grouplist);

        value["friendlist"] = friendlist;
        value["grouplist"] = grouplist;
        SendJsonString(bev, value);

        // 更新在线用户
        chatInfo_->InsertOnlineUser(bev, v);
        // chatInfo_->OnlineUser_debug(); //for debug

        // 通知所有好友(上线通知)
        // 查看好友列表中的所有好友是否存在于在线好友的哈希表中，如果存在就给发送好友上线JSon字符串
        std::vector<std::string> friends;
        size_t n = Util::Split(friendlist, "|", &friends);
        // for(auto s: friends)
        // {
        //     std::cout << s << std::endl;
        // }
        //for debug 

        for (int i = 0; i < n; i++)
        {
            std::string username = friends[i];
            // 查看当前好友是否在线
            struct bufferevent *friendBev = chatInfo_->IsOnlineUser(username);
            if (friendBev == nullptr)
            {
                // 好友不在线
                continue;
            }

            // 好友在线,则发送好友上线Json
            Json::Value value;
            value["cmd"] = "online";
            value["username"] = v["username"].asString();

            SendJsonString(friendBev, value);
        }
    }

    // 添加好友模块
    void HandleAddFriend(struct bufferevent *bev, const Json::Value &v)
    {
        // 判断好友是否存在
        if (database_->UserExist(v["friend"].asString()) == false)
        {
            // 好友不存在
            Json::Value value;
            value["cmd"] = "addfriend_reply";
            value["result"] = "not_exist";
            value["friend"] = v["friend"].asString();

            SendJsonString(bev, value);
            return;
        }
        // 判断是否好友关系
        if (database_->IsFriend(v["username"].asString(), v["friend"].asString()) == true)
        {
            // 已经是好友了
            Json::Value value;
            value["cmd"] = "addfriend_reply";
            value["result"] = "already_friend";
            value["friend"] = v["friend"].asString();

            SendJsonString(bev, value);
            return;
        }

        // 修改chatuser的friendlist
        database_->UpDateChatUserFriendList(v["username"].asString(), v["friend"].asString());
        database_->UpDateChatUserFriendList(v["friend"].asString(), v["username"].asString());

        // 向好友发送Json，有人添加你为好友
        Json::Value value;
        value["cmd"] = "be_addfriend";
        value["friend"] = v["username"].asString();
        struct bufferevent *friendBev = chatInfo_->IsOnlineUser(v["friend"].asString());
        if (friendBev != nullptr)
        {
            // 好友在线我们才给他发送添加Json
            SendJsonString(friendBev, value);
        }

        // 回复客户端,添加好友成功
        value.clear();
        value["cmd"] = "addfriend_reply";
        value["result"] = "success";
        value["friend"] = v["friend"].asString();
        SendJsonString(bev, value);
    }

    // 私聊功能
    void HandlePrivate(struct bufferevent *bev, const Json::Value &v)
    {
        // 判断好友是否在线
        struct bufferevent *friendBev = chatInfo_->IsOnlineUser(v["tofriend"].asString());
        if (nullptr == friendBev)
        {
            // 好友不在线，返回客户端
            Json::Value value;
            value["cmd"] = "private_reply";
            value["result"] = "offline";
            value["friend"] = v["tofriend"];
            SendJsonString(bev, value);

            return;
        }

        // 好友在线，直接转发
        Json::Value value;
        value["cmd"] = "private";
        value["fromfriend"] = v["username"].asString();
        value["text"] = v["text"].asString();
        SendJsonString(friendBev, value);
    }

    // 创建群功能
    void HandleCreateGroup(struct bufferevent *bev, const Json::Value &v)
    {
        // 判断群是否存在,我们直接到哈希表中判断
        if (chatInfo_->HasGroup(v["groupname"].asString()) == true)
        {
            // 群存在,返回客户端
            Json::Value value;
            value["cmd"] = "creategroup_reply";
            value["result"] = "exist";
            value["groupname"] = v["groupname"].asString();
            SendJsonString(bev, value);
            return;
        }

        // 插入数据库表chatgroup
        database_->InsertChatGroup(v);
        // 更新用户表的群列表
        database_->UpDateChatUsetGroupList(v["owner"].asString(), v["groupname"].asString());

        // 更新哈希表
        std::vector<std::string> members;
        members.push_back(v["owner"].asString());
        chatInfo_->InsertGroupInfo(v["groupname"].asString(), members);

        // 返回客户端
        Json::Value value;
        value["cmd"] = "creategroup_reply";
        value["result"] = "success";
        value["groupname"] = v["groupname"].asString();
        SendJsonString(bev, value);
    }

    // 加入群功能
    void HandleJoinGroup(struct bufferevent *bev, const Json::Value &v)
    {
        // 判断群是否存在
        if (chatInfo_->HasGroup(v["groupname"].asString()) == false)
        {
            // 群不存在，返回客户端
            Json::Value value;
            value["cmd"] = "joingroup_reply";
            value["result"] = "not_exist";
            value["groupname"] = v["groupname"].asString();;
            SendJsonString(bev, value);
            return;
        }

        // 判断用户是否在群里
        if (chatInfo_->IsInGroup(v["username"].asString(), v["groupname"].asString()) == true)
        {
            // 已经在群里了，返回客户端
            Json::Value value;
            value["cmd"] = "joingroup_reply";
            value["result"] = "already";
            value["groupname"] = v["groupname"].asString();
            SendJsonString(bev, value);
            return;
        }

        // 修改数据库chatgroup表中的群成员列表
        database_->UpDateChatGroup(v["username"].asString(), v["groupname"].asString());

        // 更新group_info_的群成员列表
        chatInfo_->UpDateGroupInfo(v["username"].asString(), v["groupname"].asString());

        // 通知群成员，有新用户加入群聊
        std::vector<struct bufferevent *> bufferevents;
        // 获取群成员的bev
        chatInfo_->GroupInfoMemberBev(v["groupname"].asString(), &bufferevents);

        Json::Value value;
        value["cmd"] = "new_member_join";
        value["groupname"] = v["groupname"].asString();
        value["username"] = v["username"].asString();
        for (auto &memberBev : bufferevents)
        {
            if (memberBev != nullptr)
            {
                // 群成员在线，我们就发送有人加入群通知
                if(memberBev != bev){
                    //需要排除一下自己，不给自己发送，这个时候我们已经把自己添加进数据库了
                    SendJsonString(memberBev, value);
                }
            }
        }

        // 通知客户端
        value.clear();
        value["cmd"] = "joingroup_reply";
        value["result"] = "success";
        value["groupname"] = v["groupname"].asString();
        std::string memberNames;
        // 获取群聊中，群成员列表
        chatInfo_->GroupInfoMemberName(v["groupname"].asString(), &memberNames);
        value["member"] = memberNames;
        SendJsonString(bev, value);
    }

    // 群聊功能
    void HandleGroupChat(struct bufferevent *bev, const Json::Value &v)
    {
        // 获取群聊的所有成员的bev
        std::vector<struct bufferevent *> bufferevents;
        chatInfo_->GroupInfoMemberBev(v["groupname"].asString(), &bufferevents);

        // 进行转发
        for (auto &memberBev : bufferevents)
        {
            if (memberBev == nullptr)
            {
                // 如果用户不在线
                continue;
            }
            if (memberBev == bev)
            {
                // 自己不给自己发送
                continue;
            }

            Json::Value value;
            value["cmd"] = "groupchat_reply";
            value["groupname"] = v["groupname"];
            value["from"] = v["username"];
            value["text"] = v["text"];

            SendJsonString(memberBev, value);
        }
    }

    // 发送文件功能
    void HandleSendFile(struct bufferevent *bev, const Json::Value &v)
    {
        // 获取要发送好友的bev
        struct bufferevent *friendBev = chatInfo_->IsOnlineUser(v["friendname"].asString());
        if (nullptr == friendBev)
        {
            // 好友不在线，返回客户端
            Json::Value value;
            value["cmd"] = "file_reply";
            value["result"] = "offline";

            SendJsonString(bev, value);
            return;
        }

        if(v["step"] == "1"){
              //好友在线回复
            Json::Value tmp;
            tmp["cmd"] = "file_reply";
            tmp["result"] = "online";
            SendJsonString(bev, tmp);
        }
      

        Json::Value value;
        if (v["step"] == "1")
        {
            // 发送文件属性
            value["cmd"] = "file_name";
            value["filename"] = v["filename"];
            value["filelength"] = v["filelength"];
            value["fromuser"] = v["username"];
        }
        else if (v["step"] == "2")
        {
            // 发送文件内容
            value["cmd"] = "file_transfer";
            value["text"] = "helloworld";
        }
        else if (v["step"] == "3")
        {
            // 发送文件结束标志
            value["cmd"] = "file_end";
        }

        SendJsonString(bev, value);
        return;
    }

    // 下线提醒功能
    void HandleOffline(struct bufferevent *bev, const Json::Value &v)
    {
        // 从在线用户哈希表中移除
        chatInfo_->EraseOnlineUser(v["username"].asString().c_str());

        // 释放连接
        bufferevent_free(bev);

        // 通知好友
        std::string friendlist;
        std::vector<std::string> friendnames;
        database_->FriendListAndGroupList(v["username"].asString().c_str(), &friendlist, nullptr);
        size_t n = Util::Split(friendlist, "|", &friendnames);
        for (int i = 0; i < n; i++)
        {
            struct bufferevent *friendBev = chatInfo_->IsOnlineUser(friendnames[i]);
            if(nullptr == friendBev){
                //好友已下线,不发送
                continue;
            }

            Json::Value value;
            value["cmd"] = "friend_offline";
            value["username"] = v["username"];
            SendJsonString(friendBev,value);
        }

        lg(Info,"%s 已下线",v["username"].asString().c_str());
    }

    //获取群聊成员
    void HandleGetGroupMember(struct bufferevent *bev, const Json::Value &v){
        //获取群聊成员列表
        std::string groupmember;// "xxx|bbb|ccc|"
        chatInfo_->GroupInfoMemberName(v["groupname"].asString(),&groupmember);

        //构建JSon
        Json::Value value;
        value["cmd"] = "groupmember_reply";
        value["member"] = groupmember;

        //发送给客户端
        SendJsonString(bev,value);
    }

public:
    ChatThread(ChatInfo *chatInfo, ChatDataBase *dataBase)
    {
        // 创建一个事件基础对象
        base_ = event_base_new();

        // 绑定数据库对象和在线用户和群聊信息的类对象
        chatInfo_ = chatInfo;
        database_ = dataBase;

        // 创建线程
        thread_ = new std::thread(ChatThread::Worker, this);

        // 初始化线程ID
        Id_ = thread_->get_id();
    }
    ~ChatThread()
    {
        if (thread_)
            delete thread_;
    }

public:
    std::thread::id get_id() const
    {
        return Id_;
    }

    struct event_base *Event_Base() const
    {
        return base_;
    }

    // 解决粘包问题
    void StickyWrapHelper(struct bufferevent *bev, char *packet)
    {
        int size;
        size_t count = 0;

        if (bufferevent_read(bev, &size, 4) != 4)
        {
            return ;
        }

        char buf[1024] = {0};
        while (1)
        {
            count += bufferevent_read(bev, buf, size - count);
            strcat(packet, buf);
            memset(buf, 0, 1024);
            if (count >= size)
            {
                break;
            }
        }

        return ;
    }

    // 子线程监听BufferEvent触发了读事件调用的读回调
    static void ReadCallBack(struct bufferevent *bev, void *ctx)
    {
        ChatThread *thread = static_cast<ChatThread *>(ctx);

        // 读取客户端发来的数据
        char buf[1024] = {0};
        // 需要处理TCP粘包问题
        thread->StickyWrapHelper(bev, buf);

        // 打印日志
        lg(Info, "Thread-%d recv a message: %s", thread->get_id(), buf);

        // 解析收到的数据
        // 判断buf是不是一个json格式字符串
        Json::Reader reader;
        Json::Value value;
        if (!reader.parse(buf, value))
        {
            // 解析失败，buf不是json格式字符串
            lg(Warning, "data is not json");
            return;
        }

        // 执行服务
        if (value["cmd"] == "register")
        {
            // 注册
            return thread->HandleRegister(bev, value);
        }
        else if (value["cmd"] == "login")
        {
            // 登录
            return thread->HandleLogin(bev, value);
        }
        else if (value["cmd"] == "addfriend")
        {
            // 添加好友
            return thread->HandleAddFriend(bev, value);
        }
        else if (value["cmd"] == "private")
        {
            // 私聊
            return thread->HandlePrivate(bev, value);
        }
        else if (value["cmd"] == "creategroup")
        {
            // 创建群
            return thread->HandleCreateGroup(bev, value);
        }
        else if (value["cmd"] == "joingroup")
        {
            // 加入群
            return thread->HandleJoinGroup(bev, value);
        }
        else if (value["cmd"] == "groupchat")
        {
            // 群聊
            return thread->HandleGroupChat(bev, value);
        }
        else if (value["cmd"] == "file")
        {
            // 发送文件
            return thread->HandleSendFile(bev, value);
        }
        else if (value["cmd"] == "offline")
        {
            // 下线提醒
            return thread->HandleOffline(bev, value);
        }else if(value["cmd"] == "groupmember"){

            //获取群聊成员
            return thread->HandleGetGroupMember(bev,value);
        }
    }

    static void WriteCallBack(struct bufferevent *bev, void *ctx) {}
    static void EventCallBack(struct bufferevent *bev, short what, void *ctx) {
        if(what & BEV_EVENT_EOF){
            //对端关闭了连接
            bufferevent_free(bev);
        }
        else{
            lg(Warning,"Unkonw ERROR, bev:%p",bev);
        }
    }
};