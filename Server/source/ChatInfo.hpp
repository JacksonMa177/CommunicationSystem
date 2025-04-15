#pragma once

#include "Util.hpp"

#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>
#include <mutex>
#include <cassert>

#include <json/json.h>

#define MAX_GROUP_COUNT 1024    //最大群聊创建数量

//单个用户信息
class User
{
public:
    User(const std::string &user_name, struct bufferevent *bev)
        :user_name_(user_name),bev_(bev)
    {}

    std::string user_name_; //用户名
    struct bufferevent *bev_;    //用户触发的事件
};

//聊天信息类，在服务器启动时，将数据库中存放的用户信息和群聊信息，加载至内存中，以提高我们访问的速度
class ChatInfo
{
private:
    std::unordered_map<std::string,User> online_users_;  //存放在线用户信息
    std::unordered_map<std::string,std::vector<std::string>> group_info_;   //存放群聊信息，key:群聊名称 value:群聊中用户的用户名集合

    //两把互斥锁，用户保证STL线程安全 由于我们的用户触发的事件可能在不同的线程中，访问同一个STL对象，可能会有线程安全问题
    std::mutex online_users_mutex_;
    std::mutex group_info_mutex_;
public:
    ChatInfo(){
        // online_users_ = new std::unordered_map<std::string,User>;
        // group_info_ = new std::unordered_map<std::string,std::vector<std::string>>;
        online_users_.reserve(100);
    }

    ~ChatInfo(){
        // if(online_users_) delete online_users_;
        // if(group_info_) delete group_info_;
    }
public:
    //将读取到的群聊信息插入到ChatInfo类中的map中
    //groupName|member1|member2|member3
    //对字符串按照|进行分割
    //这个函数对群聊信息表进行了操作，但是我们不用对他加锁，因为这个函数是在服务区构造的时候调用的，这时候还没有创建其他线程，所以不存在线程安全问题
    void InitGroupInfo(const std::vector<std::string> &groupInfo,int groupCount){

        //一次循环就可以插入一个群聊信息进map中
        for(int i = 0; i < groupCount; i++){
            //这个数组第一个元素是群主姓名，后面的元素就是群成员姓名
            std::vector<std::string> groupMembers;

            //字符串分割  
            Util::Split(groupInfo[i],"|",&groupMembers);

            //群聊名称
            std::string groupname = groupMembers[0]; 

            //删除群聊名称元素
            groupMembers.erase(groupMembers.begin());  

            //将一个群聊信息插入进map中
            group_info_.insert(std::make_pair(groupname,groupMembers));
        }
    }

    //更新在线用户哈希表
    void InsertOnlineUser(struct bufferevent *bev,const Json::Value &v){
        std::string username = v["username"].asString();
        User s(username,bev);
        {
            std::unique_lock<std::mutex> lock(online_users_mutex_);
            online_users_.insert(std::make_pair(username,s));
        }

        lg(Info,"%s 以上线",v["username"].asString().c_str());
    }

    //从在线用户哈希表中移除，下线是使用
    void EraseOnlineUser(const std::string &username){
        {
            std::unique_lock<std::mutex> lock(online_users_mutex_);
            auto it =online_users_.find(username);
            if(it != online_users_.end()){
                online_users_.erase(username);
            }

            lg(Info," %s 已下线",username.c_str());
        }
    }

    //更新群聊哈希表（添加群时调用）,更新群聊成员
    void UpDateGroupInfo(const std::string &username,const std::string &groupname){
        {
            std::unique_lock<std::mutex> lock(group_info_mutex_);
            group_info_[groupname].push_back(username);
            //直接对哈希表中元素进行修改
            
        }
    }

    //插入一条群信息到group_info哈希表中
    void InsertGroupInfo(const std::string &groupname,std::vector<std::string> members){
        {
            std::unique_lock<std::mutex> lock(group_info_mutex_);
            group_info_.insert(std::make_pair(groupname,members));
        }
    }

    //是否存在于OnlineUser的哈希表
    struct bufferevent *IsOnlineUser(const std::string &username){
        {
            std::unique_lock<std::mutex> lock(online_users_mutex_);
            auto it = online_users_.find(username);
            if(it == online_users_.end()){
                //好友不在线
                return nullptr;
            }

            //好友在线
            return it->second.bev_;
        }
    }

    //群是否存在
    bool HasGroup(const std::string &groupname){
        {
            std::unique_lock<std::mutex> lock(group_info_mutex_);
            auto it = group_info_.find(groupname);
            if(it == group_info_.end()){
                //不存在
                return false;
            }

            //存在
            return true;
        } 
    }

    //用户是否在群中
    bool IsInGroup(const std::string &username,const std::string &groupname){
        {
            std::unique_lock<std::mutex> lock(group_info_mutex_);
            auto it = group_info_.find(groupname);
            assert(it != group_info_.end());    //群必须存在
            for(int i = 0;i < it->second.size(); i++){
                if(username == it->second[i]){
                    //用户在群中
                    return true;
                }
            }
            //用户不在群中
            return false;
        }
    }

    //将群聊中的成员的bufferevent返回
    void GroupInfoMemberBev(const std::string &groupname,std::vector<struct bufferevent*> *bufferevents){
        {
            std::unique_lock<std::mutex> lock(group_info_mutex_);
            auto it = group_info_.find(groupname);
            assert(it != group_info_.end());    //群必须存在
            for(int i = 0;i < it->second.size(); i++){
               //这里的it->second是一个vector，里面存放的是群成员的姓名
               struct bufferevent * bev = IsOnlineUser(it->second[i]);
               bufferevents->push_back(bev);
            }
        }
    }

    //将群聊中成员的username返回
    void GroupInfoMemberName(const std::string &groupname,std::string *memberNames){
        {
            std::unique_lock<std::mutex> lock(group_info_mutex_);
            auto it = group_info_.find(groupname);
            assert(it != group_info_.end());    //群必须存在
            for(int i = 0;i < it->second.size(); i++){
               //这里的it->second是一个vector，里面存放的是群成员的姓名
               *memberNames += it->second[i];
               *memberNames += "|";
            }

            // // //删除最后一个|
            // memberNames->pop_back();
        }
    }

    //for debug 遍历group_info
    void map_info_debug(){
        for(auto it = group_info_.begin(); it != group_info_.end(); it++){
            std::cout<< it->first << " ";
            for(auto iit = it->second.begin(); iit!=it->second.end();iit++){
                std::cout<< *iit << " ";
            }
            std::cout <<"\n";
        }   
        std::cout<< "----"<<std::endl;
    }

     //for debug 遍历OnlineUser_
    void OnlineUser_debug(){
        for(auto it = online_users_.begin(); it != online_users_.end(); it++){
            std::cout<< it->first << " " << it->second.bev_;
            std::cout <<"\n";
        }   
    }
};