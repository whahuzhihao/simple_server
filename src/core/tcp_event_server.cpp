#include "../include/tcp_event_server.h"
#include <sstream>

TcpEventServer::TcpEventServer(int port, int tcount, int wcount) {
    //初始化各项数据
    m_ThreadCount = tcount;
    m_WorkerCount = wcount;
    m_Port = port;
    m_MasterPid = getpid();
    m_MainBase = new ReactorThread;
//    m_Threads = new ReactorThread[m_ThreadCount];
    m_MainBase->tid = pthread_self();
    m_MainBase->base = event_base_new();

    OnClose = NULL;
    OnReceive = NULL;
    OnConnect = NULL;
    m_IfMaster = true;
}

TcpEventServer::~TcpEventServer() {
    //停止事件循环（如果事件循环没开始，则没效果）
    StopRun(NULL);

    //释放内存
    event_base_free(m_MainBase->base);
    for (int i = 0; i < m_ThreadCount; i++)
        event_base_free(m_Threads[i].base);

    delete m_MainBase;
    delete[] m_Threads;
}

bool TcpEventServer::On(const char* name, ServerCbFunc func) {
    if(strcasecmp(name, "Receive") == 0){
        OnReceive = func;
        return true;
    }else if(strcasecmp(name, "Close") == 0){
        OnClose = func;
        return true;
    }else if(strcasecmp(name, "Connect") == 0){
        OnConnect = func;
        return true;
    }
    return false;
}

//void TcpEventServer::ErrorQuit(const char *str) {
//    //输出错误信息，退出程序
//    fprintf(stderr, "%s", str);
//    if (errno != 0)
//        fprintf(stderr, " : %s", strerror(errno));
//    fprintf(stderr, "\n");
//    exit(1);
//}

void TcpEventServer::SetupThread(ReactorThread *me) {

    me->base = event_base_new();
    if (NULL == me->base)
        ErrorQuit("event base new error");

    //在主线程和子线程之间建立管道
    event_set(&me->notifyEvent, me->notifyReceiveFd,
              EV_READ | EV_PERSIST, this->ThreadProcess, me);
    event_base_set(me->base, &me->notifyEvent);
    if (event_add(&me->notifyEvent, 0) == -1)
        ErrorQuit("Can't monitor libevent notify pipe\n");
}

void TcpEventServer::InitThread(ReactorThread* me){
    me->tcpConnect = this;
    int fds[2];
    if (pipe(fds))
        ErrorQuit("create pipe error");
    me->notifyReceiveFd = fds[0];
    me->notifySendFd = fds[1];
}

void *TcpEventServer::WorkerLibevent(void *arg) {
    //开启libevent的事件循环，准备处理业务
    ReactorThread *me = (ReactorThread *) arg;
    printf("thread %u started\n", (unsigned int)me->tid);
    event_base_dispatch(me->base);
    printf("subthread done\n");
}

const char* i2s(int i)
{
    std::stringstream ss;
    ss <<  i;
    return ss.str().c_str();
}

int TcpEventServer::InitProcessPool(){
    for(int i=0;i<m_WorkerCount;i++){
        pid_t pid = fork();
        //子进程
        if(pid == 0){
            WorkerProcessing();
            return 1;
        }
    }
    return 0;
}

int TcpEventServer::SendMsgQueue(const EventData* str, int len, long mtype){
    MsgBuff msgbuff;
    memcpy(msgbuff.data, str, len);
    msgbuff.mtype = mtype;
    return msgsnd(m_MsgId,  &msgbuff,  sizeof(msgbuff.data),  IPC_NOWAIT);
}
int TcpEventServer::ReceiveMsgQueue(MsgBuff &msgbuf){
    memset(msgbuf.data,  '\0',  sizeof(msgbuf.data));
    return msgrcv(m_MsgId,  &msgbuf,  sizeof(msgbuf.data), msgbuf.mtype,  0);
}

void TcpEventServer::WorkerProcessing(){
    m_IfMaster = false;
    MsgBuff msgbuf;
    msgbuf.mtype = m_MasterPid;
    while(1){
        int ret = ReceiveMsgQueue(msgbuf);
        printf("%s\n", msgbuf.data);
        EventData *data = (EventData *)&(msgbuf.data);
        printf("receive from reactor:%d |%d| %d| %u| %s",data->info.fd,data->info.type, data->info.pipefd,(unsigned int)data->info.from_id,data->data);
        if(data->info.type == SENDTOWORKER){
            printf("now send to reactor\n");
            EventData tmp;
            DataHead head;
            head.type = SENDTOREACTOR;
            tmp.info = head;
            int res = write(m_Threads[0].notifySendFd, &tmp, sizeof tmp);
            printf("send result:%d\n",res);
        }
//        server_get_object(GetServerObject())->Send(data->info.fd, data->data);
//        if(OnReceive){
//            OnReceive();
//        }
//        for(int i=0;i<m_ThreadCount;i++){
//            if((uint32_t)m_Threads[i].tid == data->info.from_id){
//                int r = m_Threads[i].tcpConnect->Send(data->info.fd, "abcdefg");
//                printf("send result %d\n",r);
//            }
//        }
    }
}

bool TcpEventServer::BeforeRun(){
    pthread_mutex_init(&m_Lock,NULL);
    m_Threads = new ReactorThread[m_ThreadCount];
    printf("masterPid:%d\n",m_MasterPid);
    key_t key = ftok(i2s(m_MasterPid), 'a');
    //创建主进程跟子进程通信的消息队列
    m_MsgId = msgget(key,  IPC_CREAT|0777);
    printf("msgQueueId:%d\n",m_MsgId);


    //分配pipe 这里在fork子进程之前进行
    for (int i = 0; i < m_ThreadCount; i++) {
        InitThread(&m_Threads[i]);
    }

    //初始化进程
    if(InitProcessPool() == 1){
        return false;
    }

    //初始化各个子线程的结构体
    for (int i = 0; i < m_ThreadCount; i++) {
        SetupThread(&m_Threads[i]);
    }
    return true;
}

bool TcpEventServer::StartRun() {
    //子进程不往下跑了
    if(BeforeRun() == false){
        return true;
    }

    //主线程开始监听tcp端口
    evconnlistener *listener;
    //如果端口号不是EXIT_CODE，就监听该端口号
    if (m_Port != EXIT_CODE) {
        sockaddr_in sin;
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons(m_Port);
        listener = evconnlistener_new_bind(m_MainBase->base,
                                           ListenerEventCb, (void *) this,
                                           LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE | LEV_OPT_THREADSAFE, -1,
                                           (sockaddr *) &sin, sizeof(sockaddr_in));
        if (NULL == listener)
            ErrorQuit("TCP listen error");
    }

    //开启各个子线程
    for (int i = 0; i < m_ThreadCount; i++) {
        pthread_create(&m_Threads[i].tid, NULL,
                       WorkerLibevent, (void *) &m_Threads[i]);
    }

    //开启主线程的事件循环
    event_base_dispatch(m_MainBase->base);

    //事件循环结果，释放监听者的内存
    if (m_Port != EXIT_CODE) {
        //printf("free listen\n");
        evconnlistener_free(listener);
    }
}

void TcpEventServer::StopRun(timeval *tv) {
    int contant = EXIT_CODE;
    //向各个子线程的管理中写入EXIT_CODE，通知它们退出
    for (int i = 0; i < m_ThreadCount; i++) {
        write(m_Threads[i].notifySendFd, &contant, sizeof(int));
    }
    //结果主线程的事件循环
    event_base_loopexit(m_MainBase->base, tv);
}

void TcpEventServer::ListenerEventCb(struct evconnlistener *listener,
                                     evutil_socket_t fd,
                                     struct sockaddr *sa,
                                     int socklen,
                                     void *user_data) {
    TcpEventServer *server = (TcpEventServer *) user_data;

    //随机选择一个子线程，通过管道向其传递socket描述符
    int num = rand() % server->m_ThreadCount;
    int sendfd = server->m_Threads[num].notifySendFd;
    DataHead head;
    head.fd = fd;
    head.type = NEWCONN;
    EventData data;
    data.info = head;
//    write(sendfd, &fd, sizeof(evutil_socket_t));
    write(sendfd, &data, sizeof(data));
}

void TcpEventServer::ThreadProcess(int fd, short which, void *arg) {
    ReactorThread *me = (ReactorThread *) arg;

    //从管道中读取数据（socket的描述符或操作码）
    int pipefd = me->notifyReceiveFd;
    evutil_socket_t socketfd;
    EventData data;
//    read(pipefd, &socketfd, sizeof(evutil_socket_t));
    read(pipefd, &data, sizeof(data));
    //主线程分配来的新连接
    if(data.info.type == NEWCONN){
        socketfd = data.info.fd;
        //如果操作码是EXIT_CODE，则终于事件循环
        if (EXIT_CODE == socketfd) {
            event_base_loopbreak(me->base);
            return;
        }

        //新建连接
        struct bufferevent *bev;
        bev = bufferevent_socket_new(me->base, socketfd, BEV_OPT_CLOSE_ON_FREE);
        if (!bev) {
            fprintf(stderr, "Error constructing bufferevent!");
            event_base_loopbreak(me->base);
            return;
        }

        //将该链接放入队列
        Conn *conn = new Conn(socketfd);
        conn->m_Thread = me;
        me->tcpConnect->AddConn(socketfd, conn);

        //准备从socket中读写数据
        bufferevent_setcb(bev, ReadEventCb, WriteEventCb, CloseEventCb, conn);
        bufferevent_enable(bev, EV_WRITE | EV_PERSIST);
        bufferevent_enable(bev, EV_READ | EV_PERSIST);

        //调用用户自定义的连接事件处理函数
        me->tcpConnect->ConnectionEvent(conn);
    }else if(data.info.type == SENDTOREACTOR){
        //接受worker进程发来的消息
        printf("shoudao sendto reactor\n");
    }

}

void TcpEventServer::ReadEventCb(struct bufferevent *bev, void *data) {
    Conn *conn = (Conn *) data;

    conn->m_ReadBuf = bufferevent_get_input(bev);
    conn->m_WriteBuf = bufferevent_get_output(bev);

    //调用用户自定义的读取事件处理函数
    conn->m_Thread->tcpConnect->ReadEvent(conn);
}

void TcpEventServer::WriteEventCb(struct bufferevent *bev, void *data) {
    Conn *conn = (Conn *) data;
    conn->m_ReadBuf = bufferevent_get_input(bev);
    conn->m_WriteBuf = bufferevent_get_output(bev);

    //调用用户自定义的写入事件处理函数
    conn->m_Thread->tcpConnect->WriteEvent(conn);

}

void TcpEventServer::CloseEventCb(struct bufferevent *bev, short events, void *data) {
    Conn *conn = (Conn *) data;
    //调用用户自定义的断开事件处理函数
    conn->m_Thread->tcpConnect->CloseEvent(conn, events);
    bufferevent_free(bev);
    conn->m_Thread->tcpConnect->DelConn(conn->GetFd());
}

int TcpEventServer::Send(int fd, char *str) {
    if(m_IfMaster){
        Conn *conn = GetConn(fd);
        if(conn != NULL){
            return conn->AddToWriteBuffer(str, strlen(str));
        }
    }else{
        //不是master
    }

    return 0;
}

bool TcpEventServer::AddSignalEvent(int sig, void (*ptr)(int, short, void *)) {
    //新建一个信号事件
    event *ev = evsignal_new(m_MainBase->base, sig, ptr, (void *) this);
    if (!ev ||
        event_add(ev, NULL) < 0) {
        event_del(ev);
        return false;
    }

    //删除旧的信号事件（同一个信号只能有一个信号事件）
    DeleteSignalEvent(sig);
    m_SignalEvents[sig] = ev;

    return true;
}

bool TcpEventServer::DeleteSignalEvent(int sig) {
    map<int, event *>::iterator iter = m_SignalEvents.find(sig);
    if (iter == m_SignalEvents.end())
        return false;

    event_del(iter->second);
    m_SignalEvents.erase(iter);
    return true;
}

event *TcpEventServer::AddTimerEvent(void (*ptr)(int, short, void *),
                                     timeval tv, bool once) {
    int flag = 0;
    if (!once)
        flag = EV_PERSIST;

    //新建定时器信号事件
    event *ev = new event;
    event_assign(ev, m_MainBase->base, -1, flag, ptr, (void *) this);
    if (event_add(ev, &tv) < 0) {
        event_del(ev);
        return NULL;
    }
    return ev;
}

bool TcpEventServer::DeleteTimerEvent(event *ev) {
    int res = event_del(ev);
    return (0 == res);
}