#ifndef __HTTPSERVER_H__
#define __HTTPSERVER_H__


#include<iostream>
#include"ProtocolUtil.hpp"
#include"ThreadPool.hpp"

class HttpServer
{
    private:
        int port;
        int listen_sock;
        ThreadPool pool;
    public:
        HttpServer(int _port)
            :port(_port)
             ,listen_sock(-1)
             ,pool(5)
    {}

        void InitServer()
        {
            listen_sock=SocketApi::Socket();
            SocketApi::Bind(listen_sock,port);
            SocketApi::Listen(listen_sock);
            pool.InitThreadPool();
        }

        void Start()
        {
            for(;;)
            {
                std::string peer_ip;
                int peer_port;
                
                int sock=SocketApi::Accept(listen_sock,peer_ip,peer_port);
                if(sock>=0)
                {
                    std::cout<<peer_ip<<":"<<peer_port<<std::endl;
                    Task t(sock,Entry::HandlerRequest);
                    pool.PushTask(t);
                   //创建线程处理数据
                   //这里只解决网络上的问题:即怎么获得新链接和发出去的问题,不涉及数据的分析和业务处理,协议和数据的分析放在PROTOCOLUTIL文件中
                  //  pthread_t tid;
                   // int ret=pthread_create(&tid,NULL,Entry::HandlerRequest,(void*)sockp);
                }
            }
        }


        ~HttpServer()
        {
            if(listen_sock>0)
            {
                close(listen_sock);
            }
        }
};






#endif
