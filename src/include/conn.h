//
// Created by 胡支昊 on 2017/6/23.
//

#ifndef SIMPLE_SERVER_CONN_H
#define SIMPLE_SERVER_CONN_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <strings.h>
#include <pthread.h>
#include <fcntl.h>
#include <map>
#include "php.h"
using std::map;

class TcpEventServer;


//每个子线程的线程信息
struct ReactorThread {
//    pthread_t tid;                //线程的ID
    pthread_t tid;                //线程的ID
    struct event_base *base;    //libevent的事件处理机
    struct event notifyEvent;    //监听管理的事件机
    int notifyReceiveFd;        //管理的接收端
    int notifySendFd;            //管道的发送端

    TcpEventServer *tcpConnect;     //TcpEventServer类的指针
};

//链接操作类
class Conn {
    friend class TcpEventServer;

private:
    const int m_fd;                //socket的ID
    evbuffer *m_ReadBuf;        //读数据的缓冲区
    evbuffer *m_WriteBuf;        //写数据的缓冲区

    ReactorThread *m_Thread;    //对应的线程结构体

    Conn(int fd) : m_fd(fd) {}

    ~Conn() {}

public:
    ReactorThread *GetThread() {
        return m_Thread;
    }

    int GetFd() {
        return m_fd;
    }

    //获取可读数据的长度
    int GetReadBufferLen() {
        return evbuffer_get_length(m_ReadBuf);
    }

    //从读缓冲区中取出len个字节的数据，存入buffer中，若不够，则读出所有数据
    //返回读出数据的字节数
    int GetReadBuffer(char *buffer, int len) {
        return evbuffer_remove(m_ReadBuf, buffer, len);
    }

    //从读缓冲区中复制出len个字节的数据，存入buffer中，若不够，则复制出所有数据
    //返回复制出数据的字节数
    //执行该操作后，数据还会留在缓冲区中，buffer中的数据只是原数据的副本
    int CopyReadBuffer(char *buffer, int len) {
        return evbuffer_copyout(m_ReadBuf, buffer, len);
    }

    //获取可写数据的长度
    int GetWriteBufferLen() {
        return evbuffer_get_length(m_WriteBuf);
    }

    //将数据加入写缓冲区，准备发送
    int AddToWriteBuffer(char *buffer, int len) {
        return evbuffer_add(m_WriteBuf, buffer, len);
    }

    //将读缓冲区中的数据移动到写缓冲区
    void MoveBufferData() {
        evbuffer_add_buffer(m_WriteBuf, m_ReadBuf);
    }

    //读取全部可读数据 外部注意要释放
    char* GetAllReadBuffer(){
        int len = GetReadBufferLen();
        //TODO 先动态申请吧
        char *result = (char *)emalloc(sizeof(char) * (len + 1));
        GetReadBuffer(result, len);
        //用emalloc会存在问题 这里给末尾补充一个\0
        result[len] = '\0';
        return result;
    }

};


#endif //SIMPLE_SERVER_CONN_H
