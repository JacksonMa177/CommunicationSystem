#pragma once

#include <json/json.h>

#include "Log.hpp"
#include <mysql/mysql.h>
#include <iostream>
#include <vector>
#include <mutex>

class ChatDataBase
{
private:
    MYSQL *mysql_;     // mysql操作句柄
    std::mutex mutex_; // 对mysql操作需要加锁

public:
    ChatDataBase()
    {
    }
    ~ChatDataBase()
    {
    }

public:
    // 创建一个句柄，并连接数据库 创建了句柄之后，数据库就会在内存中申请空间，而我们对数据库的访问不多，这块空间会有点浪费，所以我们不在构造函数湖中创建句柄，而在需要连接数据库时创建句柄
    // 用完数据库之后，马上释放，关闭连接。
    bool DataBaseConnect()
    {
        mysql_ = mysql_init(nullptr);
        if (nullptr == mysql_)
        {
            lg(Fatal, "mysql_init failed:%s", mysql_error(mysql_));
            exit(1);
        }

        // 连接数据库
        mysql_ = mysql_real_connect(mysql_, "localhost", "root", "123456", "chat_database", 0, nullptr, 0);
        if (nullptr == mysql_)
        {
            lg(Fatal, "mysql_real_connect failed:%s", mysql_error(mysql_));
            // std::cout << mysql_error(mysql_) << std::endl;
            mysql_close(mysql_);
            exit(2);
        }

        // 设置字符编码
        mysql_query(mysql_, "set names utf8");

        return true;
    }

    // 关闭数据库
    void DataBaseDisConnect()
    {
        mysql_close(mysql_);
    }

public:
    // 读取群聊信息表,将读取到的查询结果构造字符串数组通过形参带出
    int ReadgroupInfo(std::vector<std::string> *groupInfo)
    {
        // 1.连接数据库
        DataBaseConnect();

        // 加锁操作
        std::unique_lock<std::mutex> lock(mutex_);
        // 2.读取群聊表中的数据
        // 2.1执行查询
        if (mysql_query(mysql_, "select * from chatgroup;") != 0)
        {
            lg(Fatal, "select * from chatgroup failed");
            return -1;
        }

        // 2.2获取查询结果 MYSQL_RES是一个二位数组
        MYSQL_RES *res = mysql_store_result(mysql_);
        if (nullptr == res)
        {
            lg(Fatal, "mysql_store_result chatgroup table failed");
            return -1;
        }

        // 2.3将查询到的结果构造字符串数组
        MYSQL_ROW row;
        int count = 0;
        while (row = mysql_fetch_row(res))
        {
            // 这里需要循环获取每一行，每一行都是一个数组
            //TODO 这里如果数据为空，会发生段错误
            (*groupInfo)[count] += row[0]; // 群聊名称
            (*groupInfo)[count] += "|";    // 分隔符
            (*groupInfo)[count] += row[2]; // 群成员

            count++;
        }

        // 关闭数据库
        DataBaseDisConnect();
        // 返回群聊的数量
        return count;
    }

    // 创建用户表和群聊表
    bool CreateTable()
    {
        // 连接数据库
        DataBaseConnect();

        // 加锁操作
        std::unique_lock<std::mutex> lock(mutex_);
        // 创建用户表
        std::string user = "create table if not exists chatuser(\
                            username varchar(20),\
                            password varchar(20),\
                            friendlist varchar(8192),\
                            grouplist varchar(8192)\
                            )charset utf8;";

        if (mysql_query(mysql_, user.c_str()) != 0)
        {
            lg(Fatal, "create chatuser table failed");
            return false;
        }

        // 创建群聊表
        std::string group = "create table if not exists chatgroup(\
                            groupname varchar(20),\
                            groupowner varchar(11),\
                            groupmember varchar(8192)\
                            )charset utf8;";

        if (mysql_query(mysql_, user.c_str()) != 0)
        {
            lg(Fatal, "create chatgroup table failed");
            return false;
        }

        // 关闭数据库
        DataBaseDisConnect();

        lg(Info, "create table success");
        return true;
    }

    // 判断用户是否存在 存在返回true,不存在返回false
    bool UserExist(const std::string &username)
    {
        // 打开数据库
        DataBaseConnect();

        char buf[1024];
        snprintf(buf, 1023, "select * from chatuser where username like '%s';", username.c_str());

        // 加锁操作
        std::unique_lock<std::mutex> lock(mutex_);

        // 执行sql语句
        if (mysql_query(mysql_, buf) != 0)
        {
            lg(Warning, " UserExist::mysql_query failed");
            DataBaseDisConnect();
            return true;
        }

        // 获取查询结果
        MYSQL_RES *res = mysql_store_result(mysql_);
        if (nullptr == res)
        {
            lg(Warning, "UserExist::mysql_store_result failed");
            DataBaseDisConnect();
            return true;
        }

        // 获取一行数据
        MYSQL_ROW row = mysql_fetch_row(res);
        if (nullptr == row)
        {
            // 用户不存在
            DataBaseDisConnect();
            return false;
        }

        // 关闭数据库
        DataBaseDisConnect();
        // 用户存在
        return true;
    }

    // 将用户信息插入用户信息表中
    bool InsertChatUser(const Json::Value &value)
    {
        // 打开数据库
        DataBaseConnect();

        std::string username = value["username"].asString();
        std::string password = value["password"].asString();

        char sql[256] = {0};
        sprintf(sql, "insert into chatuser (username, password) values ('%s', '%s');", username.c_str(), password.c_str());

        // 加锁操作
        std::unique_lock<std::mutex> lock(mutex_);

        if (mysql_query(mysql_, sql) != 0)
        {
            lg(Warning, "InsertUserTable::mysql_query failed");
            DataBaseDisConnect();
            return false;
        }

        // 关闭数据库
        DataBaseDisConnect();

        lg(Info, "Insert username:%s into the database", username.c_str());
        return true;
    }

    //将群信息插入chatgroup表
    void InsertChatGroup(const Json::Value &v){
         // 打开数据库
        DataBaseConnect();

        char sql[256] = {0};
        sprintf(sql, "insert into chatgroup  values('%s','%s','%s');",v["groupname"].asString().c_str(),v["owner"].asString().c_str(),v["owner"].asString().c_str());

        // 加锁操作
        std::unique_lock<std::mutex> lock(mutex_);

        if (mysql_query(mysql_, sql) != 0)
        {
            lg(Warning, "InsertChatGroup::mysql_query failed: %s", mysql_error(mysql_));
            DataBaseDisConnect();
            return; // 添加return，防止继续执行
        }

        lg(Info, "Insert groupname:%s into the database", v["groupname"].asString().c_str());
        // 关闭数据库
        DataBaseDisConnect();
    }

    // 验证密码正确性
    bool AuthenticationPassword(const Json::Value &v)
    {
        // 打开数据库
        DataBaseConnect();

        char buf[1024];
        snprintf(buf, 1023, "select password from chatuser where username like '%s';", v["username"].asString().c_str());

        // 加锁操作
        std::unique_lock<std::mutex> lock(mutex_);

        // 执行sql语句
        if (mysql_query(mysql_, buf) != 0)
        {
            lg(Warning, "AuthenticationPassword::mysql_query failed");
            DataBaseDisConnect();
            return false;
        }

        // 获取sql执行结果
        MYSQL_RES *res = mysql_store_result(mysql_);
        if (nullptr == res)
        {
            lg(Warning, "AuthenticationPassword::mysql_store_result failed");
            DataBaseDisConnect();
            return false;
        }

        // 获取密码信息进行比对
        MYSQL_ROW row = mysql_fetch_row(res);
        if (nullptr == row)
        {
            lg(Warning, "AuthenticationPassword::mysql_fetch_row failed");
            DataBaseDisConnect();
            return false;
        }

        // 数据库中存储的密码
        std::string password = row[0];
        if (password != v["password"].asString())
        {
            // 密码不正确
            DataBaseDisConnect();
            return false;
        }

        //密码正确
        DataBaseDisConnect();
        return true;
    }

    //返回好友列表和群列表
    void FriendListAndGroupList(const std::string &username,std::string *friendlist,std::string *grouplist){
        // 打开数据库
        DataBaseConnect();

        char buf[1024];
        snprintf(buf, 1023, "select * from chatuser where username like '%s';", username.c_str());

        // 加锁操作
        std::unique_lock<std::mutex> lock(mutex_);

        // 执行sql语句
        if (mysql_query(mysql_, buf) != 0)
        {
            lg(Warning, "FriendListAndGroupList::mysql_query failed");
            DataBaseDisConnect();
        }

        // 获取sql执行结果
        MYSQL_RES *res = mysql_store_result(mysql_);
        if (nullptr == res)
        {
            lg(Warning, "FriendListAndGroupList::mysql_store_result failed");
            DataBaseDisConnect();
        }

        
        MYSQL_ROW row = mysql_fetch_row(res);
        if (nullptr == row)
        {
            lg(Warning, "FriendListAndGroupList::mysql_fetch_row failed");
            DataBaseDisConnect();
        }

        if(row[2] == nullptr){
            //好友列表是空的
            if(friendlist)
                *friendlist = "";    
        }else{
            if(friendlist)
                *friendlist = row[2];
        }
        if(row[3] == nullptr){
            if(grouplist)
                *grouplist = "";
        }else{
            if(grouplist)
                *grouplist = row[3];
        }

        //关闭数据库
        DataBaseDisConnect();
    }

    //判断两个用户是不是好友
    bool IsFriend(const std::string user1name,const std::string &user2name){
        //获取两个用户的好友列表
        std::string user1FriendList;
        std::string user2FriendList;
        FriendListAndGroupList(user1name,&user1FriendList,nullptr);
        FriendListAndGroupList(user2name,&user2FriendList,nullptr);

        //两个用户的好友列表中必须同时存在对方的名字
        if(user1FriendList.find(user2name) != std::string::npos && user2FriendList.find(user1name) != std::string::npos){
            //是好友
            return true;
        }

        //不是好友
        return false;
    }

    //更新好友列表
    void UpDateChatUserFriendList(const std::string &username,const std::string &friendname){
        // 打开数据库
        DataBaseConnect();

        char buf[1024];
        snprintf(buf, 1023, "update chatuser set friendlist=CONCAT(IFNULL(friendlist,''),'|%s' )  where username like '%s';",friendname.c_str(),username.c_str());

        // 加锁操作
        std::unique_lock<std::mutex> lock(mutex_);

        // 执行sql语句
        if (mysql_query(mysql_, buf) != 0)
        {
            lg(Warning, "UpDateFriendList mysql_query failed");
            DataBaseDisConnect();
            return;
        }

        lg(Debug,"%s 添加好友 %s ,成功",username.c_str(),friendname.c_str());
        DataBaseDisConnect();
    }

    //更新chatGroup表的groupmember
    void UpDateChatGroup(const std::string &username,const std::string &groupname){
        // 打开数据库
        DataBaseConnect();

        char buf[1024];
        snprintf(buf, 1023, "update chatgroup set groupmember=CONCAT(IFNULL(groupmember,''),'|%s' )  where groupname like '%s';",username.c_str(),groupname.c_str());

        // 加锁操作
        std::unique_lock<std::mutex> lock(mutex_);

        // 执行sql语句
        if (mysql_query(mysql_, buf) != 0)
        {
            lg(Warning, "UpDateChatGroup mysql_query failed");
            DataBaseDisConnect();
            return;
        }

        DataBaseDisConnect();
    }

    //更新群聊列表
    void UpDateChatUsetGroupList(const std::string &username,const std::string &groupname){
        // 打开数据库
        DataBaseConnect();

        char buf[1024];
        snprintf(buf, 1023, "update chatuser set grouplist=CONCAT(IFNULL(grouplist,''),'|%s' )  where username like '%s';",groupname.c_str(),username.c_str());

        // 加锁操作
        std::unique_lock<std::mutex> lock(mutex_);

        // 执行sql语句
        if (mysql_query(mysql_, buf) != 0)
        {
            lg(Warning, "UpDateChatUsetGroupList mysql_query failed");
            DataBaseDisConnect();
            return;
        }

        // lg(Debug,"%s 添加好友 %s ,成功",username.c_str(),friendname.c_str());
        DataBaseDisConnect();
    }
};