#pragma once

#include "ChatThread.hpp"
#include <vector>

#define THREAD_COUNT 3 // 默认创建线程数量
class ChatThreadPool
{
private:
    int threadCount_;                   // 线程数量
    int next_idx_;                      // 下一个分配线程的下标
    std::vector<ChatThread *> threads_; // 存放线程对象的数组
public:
    ChatThreadPool(ChatInfo *chatInfo,ChatDataBase *dataBase,int threadCount= THREAD_COUNT)
        : threadCount_(threadCount),
          next_idx_(0),
          threads_(threadCount_)
    {
        // 创建线程对象
        for (int i = 0; i < threadCount_; i++)
        {
            //这里我们是new出来的ChatThread对象,需要手动delete
            threads_[i] = new ChatThread(chatInfo,dataBase);
        }
    }

    ~ChatThreadPool(){
        //我们在ChatThreadPool的析构函数中，释放new出来的CHatThread，相当于把线程的生命周期交给了CHatThreadPool
        for(auto &thread : threads_){
            delete thread;
        }
    }
public:
    //分配一个线程对象的指针，采用RR轮转的方式
    ChatThread * NextThread(){
        next_idx_ = (next_idx_ + 1) % threadCount_;
        return threads_[next_idx_];
    }
};