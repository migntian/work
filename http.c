#include "http.h"


int get_line(int sock,char line[],int size)//解析HTTP请求,按行并且一个字节一个字节解析
{
    int c = 'a';
    int i = 0;
    ssize_t s = 0;
    while(i < size-1 && c != '\n')
    {
        s = recv(sock,&c,1,0);
        if(s > 0)
        {
            if(c == '\r')
            {
                recv(sock,&c,1,MSG_PEEK);//以窥探的方式查看下一个字节是不是'\n'
                if(c != '\n')
                {
                    c = '\n';
                }
                else
                {
                    recv(sock,&c,1,0);
                }
            }
            line[i++] = c;
        }
        else
        {
            break;
        }
    }
    line[i] = '\0';
    return i;
}

void clear_header(int sock)//作用:清理头部信息，直到空行
{
    char line[MAX];
    do{
        get_line(sock,line,sizeof(line));
    }while(strcmp(line,"\n") != 0);
}

void get_length(int sock, char* len)//为了得到put方法里面的书本信息长度
{
    char line[MAX];
    do{
        get_line(sock,line,sizeof(line));
        if (strncmp(line, "Content-Length:", 16) == 0)
        {
            strcpy(len, line + 16);
        }
    }while(strcmp(line,"\n") != 0);
}

void echo_error(int code,int sock)
{
    char line[MAX];
    switch(code)
    {
    case 404:
        sprintf(line,"HTTP/1.0 404 NOT FOUND\r\n");
        send(sock,line,strlen(line),0);

        sprintf(line,"\r\n");
        send(sock,line,strlen(line),0);
        break;
    default:
        break;
    }
}

int echo_put(int sock, char* path)
{
    char buff[MAX]; // save put body
    char len[16] = {0};
    get_length(sock, len);
    int length = atoi(len); // body length
    read(sock, buff, length);//读取书本信息

    int fd = open(path,O_RDONLY);
    if(fd < 0)
    {
        echo_error(404,sock);
        return;
    }
    ftruncate(fd, 0);//函数的作用是将文件清空，然后想文件里覆盖数据
    write(fd, buff, length);

    char line[MAX];
    sprintf(line,"HTTP/1.0 200 OK\r\n");
    send(sock,line,strlen(line),0);

    sprintf(line,"\r\n");
    send(sock,line,strlen(line),0);

    return 200;
    close(fd);
}

void echo_get(int sock,char *path)
{
    clear_header(sock);
    int fd = open(path,O_RDONLY);
    if(fd < 0)
    {
        echo_error(404,sock);
        return;
    }

    struct stat st;
    stat(path, &st);//可以知道文件的很多信息
    int size = st.st_size;

    char line[MAX * 10];

    sprintf(line,"HTTP/1.0 200 OK\r\n");
    send(sock,line,strlen(line),0);

    sprintf(line,"\r\n");
    send(sock,line,strlen(line),0);

    sendfile(sock,fd,NULL,size);//将得到的信息发送回sock里面完成get方式获取信息
    close(fd);
}

void *handler_request(void *arg)
{
    int web_sock = *(int*)arg;
    char line[MAX];
    char method[MAX/32];//解析出请求的方法
    char url[MAX];
    char path[MAX];
    int errcode = 200;
    int ret = 0;
    char* query_string = NULL;
    if(ret = get_line(web_sock,line,sizeof(line)) < 0)
    {
        errcode = 404;
        goto end;
    }
    int i = 0;
    int j = 0;
    while(i < sizeof(method)-1 && j < sizeof(line) && !isspace(line[j]))
    {
        method[i++] = line[j++];
    }
    method[i] = '\0';//解析出方法放在method[]中
    i = 0;
    while(j < sizeof(line) && isspace(line[j]))
    {
        j++;
    }
    while(i < sizeof(url)-1 && j < sizeof(line) && !isspace(line[j]))
    {
        url[i++] = line[j++];
    }
    url[i] = '\0';//解析出请求的资源放在url[]中

    sprintf(path,"api/v1/books%s",url);//找到url之后我们将url拼接到path[]中

    if(strcasecmp(method,"get") == 0) 
    {
        echo_get(web_sock, path);//如果是get方法，调用相应的函数
    }
    else if (strcasecmp(method, "put") == 0)
    {
        echo_put(web_sock, path);//如果是put方法，调用相应的函数
    }
    else
    {
        goto end;
    }
end:
    if(errcode != 200)
    {
        echo_error(errcode,web_sock);
    }
    close(web_sock);
}

int main(int argc,char *argv[])
{
    if(argc != 2)
    {
        printf("./server\n");//命令行输入可执行程序和端口号运行服务器
        return 1;
    }

    int listen_sock = listensock(atoi(argv[1]));//封装的listensock()函数通过绑定监听，得到监听套接字

    for(;;)
    {
        struct sockaddr_in st;
        socklen_t len = sizeof(st);        
        int sock = accept(listen_sock, (struct sockaddr*)&st, &len);//获得一个已完成连接的文件描述符
        pthread_t id = 0;
        int* arg = (int*)malloc(4);
        *arg = sock;
        pthread_create(&id,NULL,handler_request,(void *)arg);//创建线程完成对连接的响应
        pthread_detach(id);//线程分离不需要等待
    }
    return 0;
}
