#ifndef __PROTOCOLUTIL_H__
#define __PROTOCOLUTIL_H__





#include<iostream>
#include<stdlib.h>
#include<string>
#include<vector>
#include<unordered_map>
#include<sstream>
#include<algorithm>
#include<unistd.h>
#include<strings.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/sendfile.h>
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

class Util
{
    public:
        // XXXX: YYYYY
        static void MakeKV(std::string s,std::string&k,std::string&v)
        {
            std::size_t pos=s.find(": ");
            k=s.substr(0,pos);
            v=s.substr(pos+2);
        }
        //整型转字符串
        static std::string IntToString(int&x)
        {
            std::stringstream ss;
            ss<<x;
            return ss.str();
        }

        static std::string CodeToDesc(int code)
        {
            switch(code)
            {
                case 200:
                    return "OK";
                case 404:
                    return "Not Found";
                default:
                    break;
            }
            return  "Unknow";
        }
};
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

class Http_Response
{
    public:
        //基本协议字段
        std::string status_line;//响应状态行
        std::vector<std::string>response_header;
        std::string blank;
        std::string response_text;//响应正文
    private:
        int code;//响应状态码
        std::string path;
        int recource_size;
    public:
        Http_Response()
            :blank("\r\n")
             ,code(200)
             ,recource_size(0)
        {}
        int&Code()
        {
            return code;
        }

        void SetPath(std::string _path)
        {
            path=_path;
        }
        std::string&Path()
        {
            return path;
        }
        void SetRecourceSize(int rs)
        {
            recource_size=rs;
        }
        int RecourceSize()
        {
            return recource_size;
        }

        //构建响应的状态行
        void MakeStatusLine()
        {
            status_line="HTTP/1.0";
            status_line+="  ";
            status_line+=Util::IntToString(code);
            status_line+="  ";
            status_line+=Util::CodeToDesc(code);
            status_line+="\r\n";
        }
        void MakeResponseHeader()
        {
            std::string line;
            line="Content-Length: ";
            line+=Util::IntToString(recource_size);
            line+="\r\n";
            response_header.push_back(line);
            line="\r\n";
            response_header.push_back(line);
        }
        ~Http_Response()
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
        std::string path;//路径
        //int recource_size;//资源大小，用于Content-Length
        std::string query_string;//参数
        std::unordered_map<std::string,std::string>header_kv;
        bool cgi;
    public:
        Http_Request()
            :path(WEBROOT)
             ,cgi(false)
             ,blank("\r\n")
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
                    cgi=true;//如果传参，cgi是true
                    //?前是url路径，？后是参数，所以进行字符串分割
                    path+=uri.substr(0,pos);
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
                cgi=true;
                path+=uri;// wwwroot/a/b/c/d.html
            }
            
        }
        //对请求报头进行解析(全部转换成k,v结构)
        void HeaderParse()
        {
            std::string k,v;
            for(auto it=request_header.begin();it!=request_header.end();it++)
            {
                Util::MakeKV(*it,k,v);
                header_kv.insert({k,v});

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
        //判断路径是否合法
        int IsPathLegal(Http_Response*rsp)
        {
            int rs=0;
            struct stat st;
            if(stat(path.c_str(),&st)<0)
            {
                return 404;
            }
            else//文件存在
            {
                rs=st.st_size;
                if(S_ISDIR(st.st_mode))//判断是否是目录，如果是目录，返回首页
                {
                    path+="/";
                    path+=HOMEPAGE;
                    stat(path.c_str(),&st);//如果是目录，这里path拼接上了首页，所以需要重新获取一下大小
                    rs=st.st_size;
                }
                 //判断是否是可执行文件
                else if((st.st_mode&S_IXUSR)|| \
                        (st.st_mode&S_IXGRP)|| \
                        (st.st_mode&S_IXOTH))
                {
                    cgi=true;
                }
                else 
                {
                    //TODO
                }
            }
            //文件存在且合法
            rsp->SetPath(path);
            rsp->SetRecourceSize(rs);
            return 0;
        }

        //判断是否需要继续读取（即读取正文）
        bool IsNeedRecv()
        {
            return method=="POST"?true:false;
        }
        //判断是否是Cgi
        bool IsCgi()
        {
            return cgi;
        }
        //报头中的Content-Length字段，描述了正文长度
        int ContentLength()
        {
            int content_length=-1;
            std::string cl= header_kv["Content-Length"];
            std::stringstream ss(cl);
            ss>>content_length;
            return content_length;
        }
        ~Http_Request()
        {}

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
        
        //读取请求报头
        void RecvRequestHeader(std::vector<std::string>&v)
        {
            std::string line="X";
            while(line!="\n")
            {
                RecvOneline(line);//一直读。将读到的信息放入line中
                if(line!="\n")
                {
                    v.push_back(line);
                }
            }
        }

        //读取正文
        void RecvText(std::string&text,int content_length)
        {
            char c;
            for(auto i=0;i<content_length;i++)
            {
                recv(sock,&c,1,0);
                text.push_back(c);
            }
        }
        void SendStatusLine(Http_Response*rsp)
        {
            std::string&sl=rsp->status_line;
            send(sock,sl.c_str(),sl.size(),0);
        }
        void SendHeader(Http_Response*rsp) //add \n
        {
            std::vector<std::string>&v=rsp->response_header;
            for(auto it=v.begin();it!=v.end();it++)
            {
                send(sock,it->c_str(),it->size(),0);
            }
        }
        void SendText(Http_Response*rsp)
        {
            std::string& path=rsp->Path();
            int fd=open(path.c_str(),O_RDONLY);
            if(fd<0)
            {
                LOG("open file error!",WARNING);
                return;
            }
            sendfile(sock,fd,NULL,rsp->RecourceSize());
            close(fd);

        }

        ~Connect()
        {
            close(sock);
        }
};

class Entry
{
    public:
        static void ProcessNonCgi(Connect*conn,Http_Request*rq,Http_Response*rsp)
        {
            rsp->MakeStatusLine();
            rsp->MakeResponseHeader();
            //rsp->MakeResponseText(rsp);

            conn->SendStatusLine(rsp);
            conn->SendHeader(rsp); //add \n
            conn->SendText(rsp);
        }
        static void ProcessResponse(Connect*conn,Http_Request*rq,Http_Response*rsp)
        {
            if(rq->IsCgi())
            {
                //ProcessCgi();
            }
            else
            {
                ProcessNonCgi(conn,rq,rsp);
            }
            
        }
       static void*HandlerRequest(void*arg)
        {
            pthread_detach(pthread_self());
            int*sock=(int*)arg;
#ifdef _DEBUG_
            //测试：处理请求.这里先不分析,只把请求读出来
            char buff[10240];
            read(*sock,buff,sizeof(buff));
            std::cout<<"#############################################"<<std::endl;
            std::cout<<buff<<std::endl;
            std::cout<<"#############################################"<<std::endl;
#else
            Connect*conn=new Connect(*sock);
            Http_Request*rq=new Http_Request();
            Http_Response*rsp=new Http_Response();
            int &code=rsp->Code();
            conn->RecvOneline(rq->request_line);//读取一行，放入request_line中
            rq->RequestLineParse();//对request_line进行分析，把所有方法转换成大写
            
            //判断方法的合法性
            if(!rq->IsMethodLegal())
            {
                code=404;
                LOG("Request Method Is not Legal",WARNING);
                goto end;
            }
            //如果方法合法，则对请求方法进行解析(GET与POST,不同方法采用的处理方式不同)
            rq->UriParse();//对请求方法进行解析

            if(rq->IsPathLegal(rsp)!=0)
            {
                code=404;
                LOG("file is not exist!",WARNING);
                goto end;
            }

            //读取请求报头
            conn->RecvRequestHeader(rq->request_header);
            //调用请求报头的解析
            rq->HeaderParse();
            //判断是否需要继续读取
            if(rq->IsNeedRecv())
            {
                conn->RecvText(rq->request_text,rq->ContentLength());
            }

            ProcessResponse(conn,rq,rsp);

end:
            delete conn;
            delete rq;
            delete rsp;
            delete sock;

#endif
            return(void*)0;

        }
    
};



#endif
