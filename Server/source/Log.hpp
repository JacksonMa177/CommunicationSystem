#pragma once

#include <stdarg.h>
#include <iostream>
#include <time.h>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <thread>


#define SIZE 1024

#define Info 0
#define Debug 1
#define Warning 2
#define Error 3
#define Fatal 4

#define Screen 1    // 往屏幕打印
#define Onefile 2   // 往一个文件打印
#define Classfile 3 // 分文件打印

const std::string filename = "log.txt";

std::string LevelToTtring(int level)
{
    switch (level)
    {
    case Info:
        return "Info";
    case Debug:
        return "Debug";
    case Warning:
        return "Warning";
    case Error:
        return "Error";
    case Fatal:
        return "Fatal";
    default:
        return "None";
    }
}

class Log
{
public:
    Log()
    {
        printMethod = Screen; // 默认往屏幕打印
        path = "./log/";
    }

    void Enable(int method)
    {
        printMethod = method;
    }

    void PrintLog(int level, const std::string &logtxt)
    {
        switch (printMethod)
        {
        case Screen:
            std::cout << logtxt;
            break;
        case Onefile:
            printOneFile(filename, logtxt);
            break;
        case Classfile:
            printClassFile(level, logtxt);
            break;
        default:
            break;
        }
    }

    void printOneFile(const std::string &filename, const std::string &logtxt)
    {
        std::string filename_=path+filename;
        int fd = open(filename_.c_str(), O_APPEND | O_CREAT | O_WRONLY, 0666);
        if (fd < 0)
            return;
        write(fd, logtxt.c_str(), logtxt.size());
    }

    void printClassFile(int level, const std::string &logtxt)
    {
        std::string file = filename + "." + LevelToTtring(level);
        printOneFile(file, logtxt);
    }

    void operator()(int level, const char *format, ...)
    {
        // 获取本机时间
        time_t t = time(NULL);
        struct tm *ctime = localtime(&t);

        // 日志等级和时间
        char leftbuffer[SIZE] = {0};
        snprintf(leftbuffer, sizeof(leftbuffer), "[%s][%p][%d-%d-%d %d-%d-%d]", LevelToTtring(level).c_str(),
                (void*)pthread_self(),
                 ctime->tm_year + 1900, ctime->tm_mon + 1, ctime->tm_mday, ctime->tm_hour, ctime->tm_min, ctime->tm_sec);

        // 日志信息
        char rightbuffer[SIZE] = {0};
        va_list s;
        va_start(s, format); // 让s指向可变参数列表的第一个参数
        vsnprintf(rightbuffer, sizeof(rightbuffer), format, s);
        va_end(s);

        // 合并
        char logbuffer[SIZE * 3] = {0};
        snprintf(logbuffer, sizeof(logbuffer), "%s %s\n", leftbuffer, rightbuffer);

        PrintLog(level, logbuffer);
    }

private:
    int printMethod; // 打印方法
    std::string path;     // 往文件打印的文件路径
};

Log lg;