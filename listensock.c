#include "http.h"

int listensock(int port)    
{
    int sock = socket(AF_INET,SOCK_STREAM,0);
    if(sock < 0)
    {
        perror("socket error");
        return -1;
    }
    int opt = 1;
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));//防止服务器断开之后不能立刻重连

    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(port);

    if(bind(sock,(struct sockaddr*)&local,sizeof(local)) < 0)
    {
        perror("bind error");
        return -1;
    }

    if(listen(sock,5) < 0)
    {
        printf("listen error");
        return -1;
    }
    return sock;
}
