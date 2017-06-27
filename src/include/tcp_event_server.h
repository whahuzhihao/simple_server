#ifndef SIMPLE_SERVER_TCP_EVENT_SERVER_H
#define SIMPLE_SERVER_TCP_EVENT_SERVER_H

#include "conn.h"
#include "php.h"
#include "sys/types.h"
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sstream>

typedef void(*ServerCbFunc)(Conn *);
typedef struct _MsgBuff{
    long mtype;//消息类型
    char data[512];//数据
}MsgBuff;

class TcpEventServer
{
private:
    int m_ThreadCount;					//子线程数
    int m_Port;							//监听的端口
    int m_WorkerCount;                  //工作进程的数量
    ReactorThread *m_MainBase;			//主线程
    ReactorThread *m_Threads;			//子线程数组
    map<int, event*> m_SignalEvents;	//自定义的信号处理
    map<int, Conn *> m_ConnMap;           //放置fd对应的conn
    pthread_mutex_t m_Lock;               //connmap操作需要加线程互斥锁
    zval* m_ServerObject;                 //对应的php类 好蛋疼啊 指针存来存去

    pid_t m_MasterPid;
    pid_t m_ManagerPid;
    int m_MsgId;
public:
    static const int EXIT_CODE = -1;

private:
    //初始化子线程的数据
    void SetupThread(ReactorThread *thread);

    //子线程的入门函数
    static void *WorkerLibevent(void *arg);
    //（主线程收到请求后），对应子线程的处理函数
    static void ThreadProcess(int fd, short which, void *arg);
    //被libevent回调的各个静态函数
    static void ListenerEventCb(evconnlistener *listener, evutil_socket_t fd,
                                sockaddr *sa, int socklen, void *user_data);
    static void ReadEventCb(struct bufferevent *bev, void *data);
    static void WriteEventCb(struct bufferevent *bev, void *data);
    static void CloseEventCb(struct bufferevent *bev, short events, void *data);

    void AddConn(int fd, Conn* conn){
        pthread_mutex_lock(&m_Lock);
        m_ConnMap[fd] = conn;
        pthread_mutex_unlock(&m_Lock);
    }

    void DelConn(int fd){
        pthread_mutex_lock(&m_Lock);
        delete m_ConnMap[fd];
        m_ConnMap.erase(fd);
        pthread_mutex_unlock(&m_Lock);
    }

    Conn* GetConn(int fd){
        pthread_mutex_lock(&m_Lock);
        Conn* conn = m_ConnMap[fd];
        pthread_mutex_unlock(&m_Lock);
        return conn;
    }

    ServerCbFunc OnReceive;
    ServerCbFunc OnConnect;
    ServerCbFunc OnClose;

    int InitProcessPool();

    void WorkerProcessing();

    int SendMsgQueue(const char* str, int len, long type);
    int ReceiveMsgQueue(MsgBuff &buff);
protected:
    //新建连接成功后，会调用该函数
    void ConnectionEvent(Conn *conn) { if(OnConnect){ OnConnect(conn);} }

    //读取完数据后，会调用该函数
    void ReadEvent(Conn *conn) {
        int len = conn->GetReadBufferLen();
        const char *buf=conn->GetAllReadBuffer();
        SendMsgQueue(buf, len+1, m_MasterPid);
    }

    //发送完成功后，会调用该函数（因为串包的问题，所以并不是每次发送完数据都会被调用）
    void WriteEvent(Conn *conn) {  }

    //断开连接（客户自动断开或异常断开）后，会调用该函数
    void CloseEvent(Conn *conn, short events) { if(OnClose) { OnClose(conn);} }

    //发生致命错误（如果创建子线程失败等）后，会调用该函数
    //该函数的默认操作是输出错误提示，终止程序
    void ErrorQuit(const char *str){};

public:
    TcpEventServer(int port, int tcount, int wcount);
    ~TcpEventServer();

    //设置监听的端口号，如果不需要监听，请将其设置为EXIT_CODE
    void SetPort(int port)
    { m_Port = port; }

    void SetServerObject(zval *obj)
    { m_ServerObject = obj; }
    zval* GetServerObject()
    { return m_ServerObject; }
    //开始事件循环
    bool StartRun();
    //在tv时间里结束事件循环
    //否tv为空，则立即停止
    void StopRun(timeval *tv);

    int GetThreadCount(){
        return m_ThreadCount;
    }

    int GetPort(){
        return m_Port;
    }

    int Send(int fd, char* str);
    //添加和删除信号处理事件
    //sig是信号，ptr为要回调的函数
    bool AddSignalEvent(int sig, void (*ptr)(int, short, void*));
    bool DeleteSignalEvent(int sig);

//    添加和删除定时事件
//    ptr为要回调的函数，tv是间隔时间，once决定是否只执行一次
    event *AddTimerEvent(void(*ptr)(int, short, void*),
                         timeval tv, bool once);
    bool DeleteTimerEvent(event *ev);

    bool On(const char* name, ServerCbFunc cb);
};

#endif //SIMPLE_SERVER_TCP_EVENT_SERVER_H
