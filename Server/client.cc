#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <iostream>
#include <unistd.h>
#include <thread>

#include <json/json.h>

void HandleRecv(int fd)
{
    while (1)
    {
        char buf[1024] = {0};
        int n = recv(fd, buf, 1023, 0);
        if (n > 0)
        {
            buf[n] = 0;
            std::cout << buf << std::endl;
        }
    }
}

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    std::string ip = "110.40.209.26";
    uint16_t port = 8888;

    struct sockaddr_in server_info;
    memset(&server_info, 0, sizeof(server_info));
    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = inet_addr(ip.c_str());
    server_info.sin_port = htons(port);
    socklen_t len = sizeof(server_info);

    int n = connect(fd, (struct sockaddr *)&server_info, len);
    if (n < 0)
    {
        std::cerr << "connect failed %s" << strerror(errno) << std::endl;
    }

    std::thread t(HandleRecv, fd);

    // Json::Value value;
    // value["cmd"] = "groupchat";
    // value["groupname"] = "健身群";
    // value["username"] = "测试2";
    // value["text"] = "hello";

    // Json::Value value;
    // value["cmd"] = "login";
    // value["username"] = "aa";
    // value["password"] = "12345";

    
    // Json::Value value;
    // value["cmd"] = "groupmember";
    // value["groupname"] = "学习群";

    Json::Value value;
    value["cmd"] =  "file";
    value["step"] =  "2";
    value["friendname"] = "test2";
    value["text"] = "hello world";

    // Json::Value value;
    // value["cmd"] = "file";
    // value["step"] = "2";
    // value["friendname"] = "12345";

    // Json::Value value;
    // value["cmd"] = "offline";
    // value["username"] = "陈亮";

    // Json::Value value;
    // value["cmd"] = "creategroup";
    // value["groupname"] = "学习群";
    // value["owner"] = "lkm";

    // Json::Value value;
    // value["cmd"] = "addfriend";
    // value["username"] = "test11";
    // value["friend"] = "test3";

    // Json::Value value;
    // value["cmd"] = "joingroup";
    // value["username"] = "测试2";
    // value["groupname"] = "健身群";

    // Json::Value value;
    // value["cmd"] = "register";
    // value["username"] = "大家好";
    // value["password"] = "123456";

    // std::string tmp = Json::FastWriter().write(value);
    // std::string msg;
    // msg += std::to_string(tmp.size());
    // msg += '\3';
    // msg += tmp;

    // send(fd, msg.c_str(), msg.size(), 0);

    // std::string msg = "hello";
    // int s = msg.size();

    // send(fd,&s,4,0);
    // send(fd,msg.c_str(),msg.size(),0);

    std::string tmp = Json::FastWriter().write(value);
    int s = tmp.size();
    char buf[1024];
    memcpy(buf,&s,4);
    memcpy(buf + 4,tmp.c_str(),tmp.size());
    send(fd,buf,s + 4,0);

    while (1)
    {
       t.join();
    }
    return 0;
}