#ifndef __PROTOCOLUTIL_H__
#define __PROTOCOLUTIL_H__





#include<iostream>
#include<stdlib.h>
#include<string>
#include<vector>
#include<sstream>
#include<algorithm>
#include<unistd.h>
#include<strings.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define BACKLOG 5
#define BUFF_NUM 1024
#define NORMAL 0
#define WARNING 1
#define ERROR 2

#define WEBROOT "wwwroot"
#define HOMEPAGE "index.html"

const char*erroLevel[]=
{
    "Normal",
    "Warning",
    "Error"
};

//文件日志信息
void log(std::string msg,int level,std::string file,int line)
{
    std::cout<<file<<" : "<< line <<"  "<< msg <<erroLevel[level]<<std::endl;
}

#define LOG(msg,level) log(msg,level,__FILE__,__LINE__);
class SocketApi
{
    public:
        static int Socket()
        {
            int sock= socket(AF_INET,SOCK_STREAM,0);
            if(sock<0)
            {
                LOG("socket error!",ERROR);
                exit(2);                
            }
            int opt=1;
            setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
            return sock;
        }
        
        static void  Bind(int sock,int port)
        {
            struct sockaddr_in local;
            bzero(&local,sizeof(local));
            local.sin_family=AF_INET;
            local.sin_port=htons(port);
            local.sin_addr.s_addr=htonl(INADDR_ANY);
            socklen_t len=sizeof(local);
            int ret=bind(sock,(sockaddr*)&local,len);
            if(ret<0)
            {
                LOG("bind error!",ERROR);
                exit(3);
            }
        }

       static void  Listen(int sock)
        {
            int ret=listen(sock,BACKLOG);
            if(ret<0)
            {
                LOG("listen error!",ERROR);
                exit(4);
            }
        }

       static int Accept( int listen_sock,std::string&ip,int&port) //这里为了输出日志信息ip和端口号，所以传入了ip和port
       {
            struct sockaddr_in peer;//远端socket信息
            socklen_t len=sizeof(peer);
            int sock=accept(listen_sock,(struct sockaddr*)&peer,&len);
            if(sock<0)
            {
                LOG("accept error!",WARNING);
                return -1;
            }

            port=ntohs(peer.sin_port);//网络字节序转换为主机字节序
            ip=inet_ntoa(peer.sin_addr);//把网络字节序的主机地址转换成点分制十进制字符串
            return sock;
       }
};


class Connect
{
    private:
        int sock;//用于进行数据读取
    public:
        Connect(int _sock)
            :sock(_sock)
        {}
        int RecvOneline(std::string& _line)
        {
            char buff[BUFF_NUM];
            int i=0;
            char c='X';
            while(c!='\n'&&i<BUFF_NUM-1)
            {
                ssize_t s=recv(sock,&c,1,0);
                if(s>0)
                {
                    if(c=='\r')
                    {
                        recv(sock,&c,1,MSG_PEEK);//这次读是窥探，看是不是\normal
                        if(c=='\n')
                        {
                            recv(sock,&c,1,0);
                        }
                        else
                        {
                            c='\n';
                        }
                    }
                    // \r \r\n \n->\n 
                    buff[i++]=c;
                }
                else
                {
                    break;
                }
            }
            buff[i]=0;//一行读取完毕后，把最后一个字符设为\0
            _line=buff;
            return i;
        }
        ~Connect()
        {}
};


class Http_Request
{
    public:
        //基本协议字段
        std::string request_line;
        std::vector<std::string> request_header;
        std::string blank;
        std::string request_text;
    private:
        //解析字段
        std::string method;
        std::string uri;
        std::string version;
    public:
        Http_Request()
            :path(WWWROOT)
        {}
        void RequestLineParse()
        {
            //1 -> 3 分割请求行
            //GET /x/y/z HTTP/1.1\n
            std::stringstream ss(request_line);
            ss>>method>>uri>>version;
            transform(method.begin(),method.end(),method.begin(),::toupper);
        }
        //对请求方法进行解析
        void UriParse()
        {
            if(method=="GET")
            {
                std::size_t pos=uri.find("?");
                if(pos!=std::string::npos)
                {
                    //?前是url路径，？后是参数，所以进行字符串分割
                    path=uri.substr(0,pos);
                    query_string=uri.substr(pos+1);
                }
                else//没带参
                {
                    path+=uri;//wwwroot / 可能访问的是根目录，所以直接给首页
                }
                //判断是否以/结尾（即是否访问的是根目录）
                if(path[path.size()-1]=='/')
                {
                    path+=HOMEPAGE; // wwwroot/index
                }
            }
            else//POST方法
            {
                path+=uri;// wwwroot/a/b/c/d.html
            }
            
        }

        //判断方法合法性
        bool IsMethodLegal()
        {
            //Get gEt Post POst
            if(method!="GET" && method!="POST")
            {
                return false;
            }
            return true;
        }
        ~Http_Request()
        {}

};
class Entry
{
    public:
       static void*HandlerRequest(void*arg)
        {
            pthread_detach(pthread_self());
            int sock=*(int*)arg;
            delete arg;
#ifdef _DEBUG_
            //测试：处理请求.这里先不分析,只把请求读出来
            char buff[10240];
            read(sock,buff,sizeof(buff));
            std::cout<<"#############################################"<<std::endl;
            std::cout<<buff<<std::endl;
            std::cout<<"#############################################"<<std::endl;
#else

            Connect*conn=new Connect(sock);
            Http_Request*rq=new Http_Request;
            conn->RecvOneline(rq->request_line);//读取一行，放入request_line中
            rq->RequestLineParse();

            if(!rq->IsMethodLegal())
            {
                LOG("Request Method Is not Legal",WARNING);
                goto end;
            }

            rq->UriParse();//对请求方法进行解析
end:
#endif
            close(sock);
            return(void*)0;

        }
    
};



#endif
