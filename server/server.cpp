#define  NDEBUG

#include "utility.h"
#include<map>
#include "vertify.h"
#include "zhangxiaofei.hpp"

#ifndef NDEBUG
#include <time.h>
#include <sys/timeb.h>
#endif // NDEBUG

map<int ,CLIENT> clients_map;
map<int,int> map_timerfd_sockets;

#ifndef NDEBUG
struct timeb rawtime1,rawtime2;
int ms1,ms2;
unsigned long s1,s2;
int out_ms,out_s;
#endif // NDEBUG

int main(int argc, char *argv[])
{
    //服务器IP + port
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    //serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    struct rlimit rt;//资源限制符
    //设置每个进程允许打开的最大文件数
    rt.rlim_max=rt.rlim_cur=EPOLL_SIZE;
    if(setrlimit(RLIMIT_NOFILE,&rt)==-1)
    {
        perror("setrlimt error.\n");
        return -1;
    }
    //创建监听socket
    int listener = socket(PF_INET, SOCK_STREAM, 0);
    if(listener < 0)
    {
        perror("listener");
        return -1;
    }
    printf("listen socket created \n");
    //绑定地址
    if( bind(listener, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("bind error");
        return -1;
    }
    //监听
    int ret = listen(listener, 5);
    if(ret < 0)
    {
        perror("listen error");
        return -1;
    }
    printf("Start to listen: %s\n", SERVER_IP);
    //在内核中创建事件表
    int epfd = epoll_create(EPOLL_SIZE);
    if(epfd < 0)
    {
        perror("epfd error");
        return -1;
    }
    printf("epoll created, epollfd = %d\n", epfd);
    static struct epoll_event events[EPOLL_SIZE];
    //往内核事件表里添加事件
    addfd(epfd, listener, true);


    if(CreateDb()==false)
    {
        perror("creat sqlite3 error");
        return -1;
    }


    //主循环
    while(1)
    {
        //printf("wait for the epoll.\n");
        //epoll_events_count表示就绪事件的数目
        int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if(epoll_events_count < 0)
        {
            perror("epoll failure");
            break;
        }
        //处理这epoll_events_count个就绪事件
        for(int i = 0; i < epoll_events_count; ++i)
        {
            //printf("here is an event.\n");
            int sockfd = events[i].data.fd;
            //新用户连接
            if(sockfd == listener)
            {
                //printf("run into the listener.\n");
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(struct sockaddr_in);

                int clientfd = accept( listener, ( struct sockaddr* )&client_address, &client_addrLength );
                //printf("client connection from: %s : % d(IP : port), socketfd = %d \n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), clientfd);

                addfd(epfd, clientfd, true);

                // 服务端用list保存用户连接
                struct CLIENT client;
                //client.socketfd=clientfd;
                clients_map[clientfd]=client;
                //printf("Now there are %d clients in the satellite.\n\n", (int)clients_map.size());//zsd

            }
            else if(map_timerfd_sockets.find(sockfd)!=map_timerfd_sockets.end())//删除timefd时，不能删map_timefd，要不然进不来，回到socket那
            {
                int timerfd=sockfd;
                int socket=map_timerfd_sockets[timerfd];
                if(clients_map.find(socket)==clients_map.end())//找不到此timefd对应的socket对应的client
                {
                    //if(socket>0) close(socket);//socket应该是大于0的吧
                    map<int,int>::iterator map_int_int_it;
                    map_int_int_it=map_timerfd_sockets.find(timerfd);
                    if(map_int_int_it!=map_timerfd_sockets.end()) map_timerfd_sockets.erase(map_int_int_it);
                    close(timerfd);
                    delfd(epfd, timerfd, true);
                    continue;
                }
                //printf("timerfd = %d\n",sockfd);
                //int timerfd=sockfd;
                //int socket=map_timerfd_sockets[timerfd];

#ifndef NDEBUG
                printf("close socket 1 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif // NDEBUG
                //if(clients_map.find(socket)!=clients_map.end())
                //{
                char message_send[BUF_SIZE];
                bzero(message_send, BUF_SIZE);
                sprintf(message_send, "-2");
                send(socket, message_send, BUF_SIZE, 0);
                close(socket);
                //printf("delfd socket 1 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                delfd(epfd, socket, true);
                //}
                //delfd(epfd, socket, true);/////////////////////
                map<int,CLIENT>::iterator map_it;
                map_it=clients_map.find(socket);
                //printf("timeout!!!   ClientID = %d closed.\n now there are %d client in the char room\n", clients_map[socket].id, (int)clients_map.size()-1);//zsd

                if(map_it!=clients_map.end())
                {
                    printf("\n====================timeout!!!===================\nClientID = %d closed. now there are %d client in the satellite.\n\n", clients_map[socket].id, (int)clients_map.size()-1);//zsd

#ifndef NDEBUG
                    ftime(&rawtime1);
                    ms1=rawtime1.millitm;
                    s1=rawtime1.time;
#endif // NDEBUG
                    switchcaseout(makelevel(map_it->second.degree,map_it->second.hdf_type,map_it->second.bss_type));
#ifndef NDEBUG
                    ftime(&rawtime2);
                    ms2=rawtime2.millitm;
                    s2=rawtime2.time;
                    out_ms=ms2-ms1;
                    out_s=s2-s1;
                    if(out_ms<0)
                    {
                        out_ms+=1000;
                        out_s-=1;
                    }
                    printf("time of switchcaseout : s = %d    ms = %d\n",out_s,out_ms);
#endif // NDEBUG
                    /*printf("band_media_level 0 : %d\n",returnband[0]);
                    printf("band_ data_level 0 : %d\n",returnband[1]);
                    printf("band_media_level 1 : %d\n",returnband[2]);
                    printf("band_ data_level 1 : %d\n",returnband[3]);
                    printf("band_media_level 2 : %d\n",returnband[4]);
                    printf("band_ data_level 2 : %d\n",returnband[5]);*/
                    //returnmyband(searchDegree(map_it->second.id),returnband);
                    clients_map.erase(map_it);
                }
#ifndef NDEBUG
                printf("close timerfd 1 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif // NDEBUG
                close(timerfd);
#ifndef NDEBUG
                printf("delfd timerfd 1 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif // NDEBUG
                delfd(epfd, timerfd, true);
                //delfd(epfd, timerfd, true);/////////////////////
                map<int,int>::iterator map_int_int_it;
                map_int_int_it=map_timerfd_sockets.find(timerfd);
                if(map_int_int_it!=map_timerfd_sockets.end()) map_timerfd_sockets.erase(map_int_int_it);
            }
            //处理用户发来的消息
            else
            {
                //printf("run into the recv.\n");
                // buf[BUF_SIZE] receive new chat message
                char buf[BUF_SIZE];
                bzero(buf, BUF_SIZE);
                // receive message
                int len;
#ifndef NDEBUG
                printf("recv socket 1 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif // NDEBUG
                //len = recv(sockfd, buf, BUF_SIZE, 0);
                //printf("recv socket 1.1 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                if(clients_map.find(sockfd)!=clients_map.end())
                {
                    len = recv(sockfd, buf, BUF_SIZE, 0);
                }
                else
                {
#ifndef NDEBUG
                    printf("recv socket error 1.1 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif // NDEBUG
                    while(recv(sockfd, buf, BUF_SIZE, 0)>0);
#ifndef NDEBUG
                    printf("recv socket error 1.2 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif // NDEBUG
                    close(sockfd);
#ifndef NDEBUG
                    printf("delfd sockfd 1 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif // NDEBUG
                    delfd(epfd, sockfd, true);
#ifndef NDEBUG
                    printf("recv socket error 1.3 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                    printf("the num of clients in the satellite = %d\n",(int)clients_map.size());
#endif // NDEBUG
                    continue;
                }
                buf[len+1]='\0';//zsd/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                //if(buf[0]=='\0') continue;
                while(buf[0]=='\0')
                {
#ifndef NDEBUG
                    printf("recv socket 1.4 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif // NDEBUG
                    len = recv(sockfd, buf, BUF_SIZE, 0);
#ifndef NDEBUG
                    printf("recv socket 1.5 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif // NDEBUG
                    //buf[len+1]='\0';
                    if(len<1) break;
                }
#ifndef NDEBUG
                printf("recv socket 1.6 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif // NDEBUG
                //printf("recv: %s       len  =  %d  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1\n",buf,len);//zsd
                if(len <= 0) // len = 0 means the client closed connection//貌似不管用
                {
#ifndef NDEBUG
                    printf("close socket 2 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif // NDEBUG
                    //if(clients_map.find(sockfd)!=clients_map.end())
                    //{
                    close(sockfd);
#ifndef NDEBUG
                    printf("delfd sockfd 1.4 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif // NDEBUG
                    delfd(epfd, sockfd, true);
                    //}
                    //close(sockfd);

                    //delfd(epfd, sockfd, true);/////////////////////
                    //clients_list.remove(sockfd); //server remove the client
                    map<int,CLIENT>::iterator map_it;
                    map_it=clients_map.find(sockfd);
                    //printf("ClientID = %d closed.\n now there are %d client in the char room\n", clients_map[sockfd].id, (int)clients_map.size()-1);//zsd
                    if(map_it!=clients_map.end())
                    {

                        printf("ClientID = %d closed. now there are %d client in the satellite.\n", clients_map[sockfd].id, (int)clients_map.size()-1);//zsd

#ifndef NDEBUG
                        ftime(&rawtime1);
                        ms1=rawtime1.millitm;
                        s1=rawtime1.time;
#endif // NDEBUG
                        switchcaseout(makelevel(map_it->second.degree,map_it->second.hdf_type,map_it->second.bss_type));
#ifndef NDEBUG
                        ftime(&rawtime2);
                        ms2=rawtime2.millitm;
                        s2=rawtime2.time;
                        out_ms=ms2-ms1;
                        out_s=s2-s1;
                        if(out_ms<0)
                        {
                            out_ms+=1000;
                            out_s-=1;
                        }
                        printf("time of switchcaseout : s = %d    ms = %d\n",out_s,out_ms);
#endif // NDEBUG
                        /*printf("band_media_level 0 : %d\n",returnband[0]);
                        printf("band_ data_level 0 : %d\n",returnband[1]);
                        printf("band_media_level 1 : %d\n",returnband[2]);
                        printf("band_ data_level 1 : %d\n",returnband[3]);
                        printf("band_media_level 2 : %d\n",returnband[4]);
                        printf("band_ data_level 2 : %d\n",returnband[5]);*/
                        //returnmyband(searchDegree(map_it->second.id),returnband);
                        clients_map.erase(map_it);
                    }
                }
                /*else if(len < 0)//并到==0里，变成<=0
                {
                    printf("recv socket 1.7 error !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                    perror("recv error");
                    //return -1;
                    continue;
                }*/
                else
                {
#ifndef NDEBUG
                    printf("recv socket 1.8 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif // NDEBUG
                    char order[ORDER_LEN+1],message[BUF_SIZE];
                    bzero(order, ORDER_LEN+1);
                    bzero(message, BUF_SIZE);
                    strncat(order,buf,ORDER_LEN);
                    strcat(message,&buf[ORDER_LEN]);
                    //printf("order= %s\n",order);
                    if(strcmp(order,"00")==0)//接收结构体
                    {
                        struct CLIENT client;
                        //memcpy(&client,message,sizeof(CLIENT));
                        memcpy(&client,&buf[ORDER_LEN],sizeof(CLIENT));
                        client.sockfd=sockfd;
//printf("start search db.\n" );
//printf("id = %d     pwd = %d \n",client.id,client.pwd);
#ifndef NDEBUG
                        ftime(&rawtime1);
                        ms1=rawtime1.millitm;
                        s1=rawtime1.time;
#endif // NDEBUG
                        if(search(client.id,client.pwd)==false)
                        {
                            printf("===========search DB = false================\n");
                            printf("Reject the client id = %d to come in.\n\n",client.id);
#ifndef NDEBUG
                            ftime(&rawtime2);
                            ms2=rawtime2.millitm;
                            s2=rawtime2.time;
                            out_ms=ms2-ms1;
                            out_s=s2-s1;
                            if(out_ms<0)
                            {
                                out_ms+=1000;
                                out_s-=1;
                            }
                            printf("time of search : s = %d    ms = %d\n",out_s,out_ms);
#endif // NDEBUG
                            //printf("start search db end.\n" );
#ifndef NDEBUG
                            printf("close socket 3 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif // NDEBUG
                            if(clients_map.find(sockfd)!=clients_map.end())
                            {
                                close(sockfd);
#ifndef NDEBUG
                                printf("delfd sockfd 1 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif // NDEBUG
                                delfd(epfd, sockfd, true);
                            }
                            //close(sockfd);
                            continue;
                        }
#ifndef NDEBUG
                        ftime(&rawtime2);
                        ms2=rawtime2.millitm;
                        s2=rawtime2.time;
                        out_ms=ms2-ms1;
                        out_s=s2-s1;
                        if(out_ms<0)
                        {
                            out_ms+=1000;
                            out_s-=1;
                        }
                        printf("time of search : s = %d    ms = %d\n",out_s,out_ms);
#endif // NDEBUG
#ifndef NDEBUG
                        ftime(&rawtime1);
                        ms1=rawtime1.millitm;
                        s1=rawtime1.time;
#endif // NDEBUG
                        //client.degree=searchDegree(client.id);
#ifndef NDEBUG
                        ftime(&rawtime2);
                        ms2=rawtime2.millitm;
                        s2=rawtime2.time;
                        out_ms=ms2-ms1;
                        out_s=s2-s1;
                        if(out_ms<0)
                        {
                            out_ms+=1000;
                            out_s-=1;
                        }
                        printf("time of searchDegree : s = %d    ms = %d\n",out_s,out_ms);
#endif // NDEBUG
//printf("end search db.\n" );

                        //int zxf_m=0;
#ifndef NDEBUG
                        ftime(&rawtime1);
                        ms1=rawtime1.millitm;
                        s1=rawtime1.time;
#endif // NDEBUG
                        client.degree=searchDegree(client.id);
                        switchcasein(makelevel(client.degree,client.hdf_type,client.bss_type));
                        int band=returnmyband(searchDegree(client.id),returnband);
#ifndef NDEBUG
                        for(int c_i=0; c_i<12; c_i++)
                        {
                            printf("switchcasein my_count[%d] = %d\n",c_i,my_count[c_i]);
                        }
#endif // NDEBUG
#ifndef NDEBUG
                        ftime(&rawtime2);
                        ms2=rawtime2.millitm;
                        s2=rawtime2.time;
                        out_ms=ms2-ms1;
                        out_s=s2-s1;
                        if(out_ms<0)
                        {
                            out_ms+=1000;
                            out_s-=1;
                        }
                        printf("time of switchcasein : s = %d    ms = %d\n",out_s,out_ms);
#endif // NDEBUG
                        if(returnband[6]==0)//接入不成功
                        {
                            printf("================ switchcasein = false ================\n" );
                            printf("Reject the client id = %d to come in.\n\n",client.id);
                            close(sockfd);
#ifndef NDEBUG
                            printf("delfd sockfd 2 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif // NDEBUG
                            delfd(epfd, sockfd, true);
                            map<int,CLIENT>::iterator map_int_CLIENT_it=clients_map.find(sockfd);
                            if(map_int_CLIENT_it!=clients_map.end())
                            {
                                clients_map.erase(map_int_CLIENT_it);
                            }
                            continue;
                        }
                        else//接入成功
                        {
                            if(returnband[8]>0)//需要踢人
                            {
                                int num=returnband[8];
                                map<int,CLIENT>::iterator map_int_CLIENT_it;
                                for(map_int_CLIENT_it=clients_map.begin(); map_int_CLIENT_it!=clients_map.end(); ++map_int_CLIENT_it)
                                {
                                    if(map_int_CLIENT_it->second.degree==returnband[7])
                                    {
                                        if(num>0)
                                        {
                                            printf("===========need to throw the client ==========\n");
#ifndef NDEBUG
                                            ftime(&rawtime1);
                                            ms1=rawtime1.millitm;
                                            s1=rawtime1.time;
#endif // NDEBUG
                                            switchcaseout(makelevel(map_int_CLIENT_it->second.degree,map_int_CLIENT_it->second.hdf_type,map_int_CLIENT_it->second.bss_type));
#ifndef NDEBUG
                                            ftime(&rawtime2);
                                            ms2=rawtime2.millitm;
                                            s2=rawtime2.time;
                                            out_ms=ms2-ms1;
                                            out_s=s2-s1;
                                            if(out_ms<0)
                                            {
                                                out_ms+=1000;
                                                out_s-=1;
                                            }
                                            printf("time of switchcaseout : s = %d    ms = %d\n",out_s,out_ms);
#endif // NDEBUG
                                            /*printf("band_media_level 0 : %d\n",returnband[0]);
                                            printf("band_ data_level 0 : %d\n",returnband[1]);
                                            printf("band_media_level 1 : %d\n",returnband[2]);
                                            printf("band_ data_level 1 : %d\n",returnband[3]);
                                            printf("band_media_level 2 : %d\n",returnband[4]);
                                            printf("band_ data_level 2 : %d\n",returnband[5]);*/
                                            //returnmyband(searchDegree(map_int_CLIENT_it->second.id),returnband);
                                            char message_send[BUF_SIZE];
                                            bzero(message_send, BUF_SIZE);
                                            sprintf(message_send, "-3");
                                            send(map_int_CLIENT_it->first, message_send, BUF_SIZE, 0);
                                            close(map_int_CLIENT_it->first);
                                            printf("throw out the client id = %d\n",map_int_CLIENT_it->second.id);
#ifndef NDEBUG
                                            printf("delfd map_int_CLIENT_it->first 3 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif // NDEBUG
                                            delfd(epfd, map_int_CLIENT_it->first, true);
                                            //往回退一个，这样之后执行++it应该不会出问题
                                            //map<int,CLIENT>::iterator map_int_CLIENT_itt=--map_int_CLIENT_it;
                                            clients_map.erase(map_int_CLIENT_it);
                                            //map_int_CLIENT_it=map_int_CLIENT_itt;
                                            map_int_CLIENT_it=clients_map.begin();
                                            --num;
                                        }
                                        else
                                        {
                                            break;
                                        }
                                    }

                                }//for
                                printf("\n");
                            }
                            else
                            {
                                /*printf("band_media_level 0 : %d\n",returnband[0]);
                                printf("band_ data_level 0 : %d\n",returnband[1]);
                                printf("band_media_level 1 : %d\n",returnband[2]);
                                printf("band_ data_level 1 : %d\n",returnband[3]);
                                printf("band_media_level 2 : %d\n",returnband[4]);
                                printf("band_ data_level 2 : %d\n",returnband[5]);*/
                                //printf("the band for this client is %d\n",returnmyband(searchDegree(client.id),returnband));
                            }
                        }


                        clients_map[sockfd]=client;
                        printf("ClientID = %d comes.\n", clients_map[sockfd].id);
                        printf("the band for this client is %d\n",band);
                        printf("Now there are %d clients in the satellite.\n\n", (int)clients_map.size());
#ifndef NDEBUG
                        printf("live_sec = %f\n",clients_map[sockfd].life_time);
#endif // NDEBUG

                        struct timespec now;
                        if(clock_gettime(CLOCK_REALTIME,&now)==-1)
                        {
                            printf("clock_gettime error.\n");
                            return -1;
                        }
                        struct itimerspec new_value;
                        new_value.it_value.tv_sec=now.tv_sec+(int)clients_map[sockfd].life_time;
                        new_value.it_value.tv_nsec=(clients_map[sockfd].life_time-(int)clients_map[sockfd].life_time*1.0)*1000000000;
                        new_value.it_interval.tv_sec=0;
                        new_value.it_interval.tv_nsec=0;
                        int timerfd=timerfd_create(CLOCK_REALTIME,0);
                        while(timerfd==-1)
                        {
                            //printf("timerfd_create error.\n");
                            timerfd=timerfd_create(CLOCK_REALTIME,0);
                            //return -1;
                        }
                        map_timerfd_sockets[timerfd]=sockfd;
                        addfd(epfd, timerfd, true);
                        timerfd_settime(timerfd,TFD_TIMER_ABSTIME,&new_value,NULL);


                        //send back message
                        char message_send[BUF_SIZE];
                        bzero(message_send, BUF_SIZE);
                        // format message
                        sprintf(message_send, "Server received ClientID=%d 's message.\n",clients_map[sockfd].id);
#ifndef NDEBUG
                        printf("send socket 1 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif // NDEBUG
                        if(clients_map.find(sockfd)!=clients_map.end())
                        {
                            if( send(sockfd, message_send, BUF_SIZE, 0) < 0 )
                            {
                                perror("error");
                                //return -1;
                                continue;
                            }
                        }

                    }
                    else if(strcmp(order,"-1")==0)//此socket退出
                    {
#ifndef NDEBUG
                        printf("close socket 4 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif // NDEBUG
                        if(clients_map.find(sockfd)!=clients_map.end())
                        {
                            close(sockfd);
#ifndef NDEBUG
                            printf("delfd sockfd 4 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif // NDEBUG
                            delfd(epfd, sockfd, true);
                        }
                        //close(sockfd);
                        //delfd(epfd, sockfd, true);/////////////////////
                        map<int,CLIENT>::iterator map_it;
                        map_it=clients_map.find(sockfd);
                        //printf("ClientID = %d closed.\n now there are %d client in the char room\n", clients_map[sockfd].id, (int)clients_map.size()-1);//zsd
                        //clients_map.erase(map_it);
                        if(map_it!=clients_map.end())
                        {
                            printf("ClientID = %d closed. now there are %d client in the satellite.\n", clients_map[sockfd].id, (int)clients_map.size()-1);//zsd
#ifndef NDEBUG
                            ftime(&rawtime1);
                            ms1=rawtime1.millitm;
                            s1=rawtime1.time;
#endif // NDEBUG
                            switchcaseout(makelevel(map_it->second.degree,map_it->second.hdf_type,map_it->second.bss_type));
#ifndef NDEBUG
                            ftime(&rawtime2);
                            ms2=rawtime2.millitm;
                            s2=rawtime2.time;
                            out_ms=ms2-ms1;
                            out_s=s2-s1;
                            if(out_ms<0)
                            {
                                out_ms+=1000;
                                out_s-=1;
                            }
                            printf("time of switchcaseout : s = %d    ms = %d\n",out_s,out_ms);
#endif // NDEBUG
                            /*printf("band_media_level 0 : %d\n",returnband[0]);
                            printf("band_ data_level 0 : %d\n",returnband[1]);
                            printf("band_media_level 1 : %d\n",returnband[2]);
                            printf("band_ data_level 1 : %d\n",returnband[3]);
                            printf("band_media_level 2 : %d\n",returnband[4]);
                            printf("band_ data_level 2 : %d\n",returnband[5]);*/
                            //returnmyband(searchDegree(map_it->second.id),returnband);
                            clients_map.erase(map_it);
                        }
                    }
                    else
                    {
                        if(clients_map[sockfd].id==-1)
                        {
                            char message_send[BUF_SIZE];
                            bzero(message_send, BUF_SIZE);
                            sprintf(message_send, "Server says: Please send the client's info first.\n");
                            send(sockfd, message_send, BUF_SIZE, 0);
                        }
                        else
                        {
                            printf("ClientID = %d says: %s\n", clients_map[sockfd].id,message);
                            char message_send[BUF_SIZE];
                            bzero(message_send, BUF_SIZE);
                            sprintf(message_send, "Server received ClientID=%d 's message.\n",clients_map[sockfd].id);
                            send(sockfd, message_send, BUF_SIZE, 0);
                        }
                    }
                }
            }
        }
    }
    close(listener); //关闭socket
    close(epfd); //关闭内核
    return 0;
}
