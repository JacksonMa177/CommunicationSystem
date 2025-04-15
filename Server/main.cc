#include "./source/ChatServer.hpp"

int main()
{
    ChatServer server;
    server.Listen(8888);

    return 0;
}