#pragma once

#include "Log.hpp"
#include "ChatInfo.hpp"
#include "ChatDataBase.hpp"
#include "ChatThread.hpp"
#include "ChatThreadPool.hpp"
#include "Util.hpp"

#include <event.h>
#include <event2/listener.h>

#include <iostream>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define MAX_BACKLOG 1024
class ChatServer
{
private:
    struct event_base *base_; // 活跃事件的集合  封装了epoll
    struct evconnlistener *listener_;    //监听器对象   就是监听套接字
    ChatDataBase *database_;    //数据库对象，用于对数据库进行操作
    ChatInfo *chatInfo_;    // 聊天信息，用于保存当前在线用户和群聊信息
    ChatThreadPool *pool_;  //线程池
private:
    //当有新连接，为新连接创建"BufferEvent"，分发进线程池中
    void DispatchEvent(int newfd)
    {   
        ChatThread *thread = pool_->NextThread();
        
        //为新连接创建BufferEvent进行事件管理
        struct bufferevent *bev = bufferevent_socket_new(thread->Event_Base(),newfd,BEV_OPT_CLOSE_ON_FREE );

        //为bev设置对应事件回调函数
        bufferevent_setcb(bev,ChatThread::ReadCallBack,ChatThread::WriteCallBack,ChatThread::EventCallBack,thread);

        //启动bev读事件监控
        bufferevent_enable(bev,EV_READ);
    }

    //需要使用static消除this指针
    //这个函数就是服务器accept获取新连接后调用的回调函数
    static void Acceptor(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr * client, int socklen, void *tmp){
        //有了新连接 打印日志
        struct sockaddr_in *client_info = (struct sockaddr_in *)client;
        lg(Info,"new connect, fd:%d, client IP:%s, client Port:%d",fd,inet_ntoa(client_info->sin_addr),ntohs(client_info->sin_port));

        //创建事件，分发进线程池中
        ChatServer *server = static_cast<ChatServer*>(tmp); //由于Acceptor是静态函数，所以必须使用this指针来调用非静态函数DispatchEvent
        server->DispatchEvent(fd);

    }

    ////初始化群聊信息至ChatInfo的map中
    void InitGroupInfo(){
        //用于存放从数据库中读取的群聊信息
        std::vector<std::string> groupInfo(MAX_GROUP_COUNT);

        //进行数据库操作，读取群聊的信息至groupInfo字符串数组中
        int groupCount = database_->ReadgroupInfo(&groupInfo);
        
        //将读取到的群聊信息插入到ChatInfo类中的map中
        chatInfo_->InitGroupInfo(groupInfo,groupCount);  

        //打印日志
        lg(Info,"Loading group chat information succeeded,Loading Count:%d",groupCount);
    }

public:
    ChatServer() {
        //创建一个事件基础对象
        base_ = event_base_new();

        //创建一个数据库对象
        database_ = new ChatDataBase();

        //创建用户表和群聊表
        if(!database_->CreateTable()){
            exit(2);
        }

        //创建一个ChatInfo对象
        chatInfo_ = new ChatInfo();

        //初始化群聊信息至ChatInfo的map中
        InitGroupInfo();

        // for debug
        // chatInfo_->map_info_debug();

        //定义线程池对象 
        pool_ = new ChatThreadPool(chatInfo_,database_);
    
    }

    ~ChatServer(){
        if(database_) delete database_;
        if(chatInfo_) delete chatInfo_;
    }

    void Listen(int port)
    {
        //设置服务器绑定的IP和端口信息
        struct sockaddr_in server_info;
        memset(&server_info,0,sizeof(server_info));
        server_info.sin_family = AF_INET;
        server_info.sin_addr.s_addr = INADDR_ANY;
        server_info.sin_port = htons(port);
        socklen_t len =sizeof(server_info);

        //创建并绑定监听器，也就是监听套接字，同时使用epoll注册监听套接字的读事件
        //这个函数封装了socket/bind/listen/accept,当获取新连接后，会调用我们自己设置的回调函数Acceptor。
        listener_ = evconnlistener_new_bind(base_,Acceptor,this,LEV_OPT_REUSEABLE |LEV_OPT_CLOSE_ON_FREE,MAX_BACKLOG,(struct sockaddr*)&server_info,len);
        if(nullptr == listener_){
            lg(Fatal,"evconnlistener_new_bind:%s",strerror(errno));
            exit(1);
        }
        
        //开始事件循环，是一个死循环，没有监控事件时，才会退出
        event_base_dispatch(base_);

        //清理工作
        //释放监听套接字,只有当我们在evconnlistener_new_bind设置了LEV_OPT_CLOSE_ON_FREE，调用这个evconnlistener_free才会释放监听套接字
        evconnlistener_free(listener_);
        event_base_free(base_);
    }


};