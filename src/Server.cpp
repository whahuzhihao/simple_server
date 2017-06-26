#include "include/tcp_event_server.h"
#include "../php_simple_server.h"

//定义类
zend_class_entry simple_server_ce;
zend_class_entry *simple_server_class_entry_ptr;

//定义参数列表结构
ZEND_BEGIN_ARG_INFO_EX(arginfo_simple_void, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_simple_server__construct, 0, 0, 2)
                ZEND_ARG_INFO(0, serv_port)
                ZEND_ARG_INFO(0, serv_thread_num)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_simple_server_on, 0, 0, 2)
                ZEND_ARG_INFO(0, event_name)
                ZEND_ARG_INFO(0, callback)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_simple_server_send, 0, 0, 2)
                ZEND_ARG_INFO(0, fd)
                ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()


static zend_function_entry simple_server_methods[] = {
        PHP_ME(simple_server, __construct, arginfo_simple_server__construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
        PHP_ME(simple_server, __destruct, arginfo_simple_void, ZEND_ACC_PUBLIC | ZEND_ACC_DTOR)
        PHP_ME(simple_server, start, arginfo_simple_void, ZEND_ACC_PUBLIC)
        PHP_ME(simple_server, on, arginfo_simple_server_on, ZEND_ACC_PUBLIC)
        PHP_ME(simple_server, send, arginfo_simple_server_send, ZEND_ACC_PUBLIC)
        PHP_FE_END
};

PHP_METHOD(simple_server, __construct)
{
    //only cli env
    if (strcasecmp("cli", sapi_module.name) != 0)
    {
        RETURN_FALSE;
    }

    long serv_port = 2111;
    long thread_num = 2;

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ll", &serv_port,
                                         &thread_num))
    {
        return;
    }
    zval* server_object = getThis();

    zend_update_property_long(simple_server_class_entry_ptr, server_object,
                              ZEND_STRL("port"), serv_port TSRMLS_CC);
    zend_update_property_long(simple_server_class_entry_ptr, server_object,
                              ZEND_STRL("threadNum"), thread_num TSRMLS_CC);

    TcpEventServer *tcpEventServer = new TcpEventServer(serv_port, thread_num);
    tcpEventServer->SetPort(serv_port);
    tcpEventServer->SetServerObject(getThis());
    server_set_object(server_object, tcpEventServer);
}

PHP_METHOD(simple_server, __destruct){
    zval* server_object = getThis();
    TcpEventServer *tcpEventServer = (TcpEventServer *)server_get_object(server_object);
    delete tcpEventServer;
    server_del_object(server_object);
}

PHP_METHOD(simple_server, start)
{
    zval* server_object = getThis();
    TcpEventServer *tcpEventServer = (TcpEventServer *)server_get_object(server_object);
    tcpEventServer->StartRun();
}

PHP_METHOD(simple_server, on)
{
    zval* server_object = getThis();
    TcpEventServer *tcpEventServer = (TcpEventServer *)server_get_object(server_object);

    zval *zname = NULL;
    zval *cb = NULL;
    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS()TSRMLS_CC, "zz", &zname, &cb))
    {
        return;
    }
    convert_to_string(zname);
    char *name = Z_STRVAL_P(zname);
    if(strcasecmp(name, "receive") == 0){
        zend_update_property(simple_server_class_entry_ptr, getThis(), ZEND_STRL("onReceive"), cb TSRMLS_CC);
        tcpEventServer->On("receive", simple_server_onreceive);
    }else if(strcasecmp(name, "connect") == 0){
        zend_update_property(simple_server_class_entry_ptr, getThis(), ZEND_STRL("onConnect"), cb TSRMLS_CC);
        tcpEventServer->On("receive", simple_server_onconnect);
    }else if(strcasecmp(name, "close")) {
        zend_update_property(simple_server_class_entry_ptr, getThis(), ZEND_STRL("onClose"), cb TSRMLS_CC);
        tcpEventServer->On("receive", simple_server_onclose);
    }

    if (check_callable(cb TSRMLS_CC) < 0)
    {
        return ;
    }
}

PHP_METHOD(simple_server, send)
{

    zval* server_object = getThis();
    TcpEventServer *tcpEventServer = (TcpEventServer *)server_get_object(server_object);

    zval *zfd = NULL;
    zval *zdata = NULL;

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS()TSRMLS_CC, "zz", &zfd, &zdata))
    {
        return;
    }
    convert_to_long(zfd);
    convert_to_string(zdata);

    char *data = Z_STRVAL_P(zdata);
    int fd = Z_LVAL_P(zfd);
    tcpEventServer->Send(fd, data);
}


static void simple_server_onreceive(Conn *conn){
    zval *zfd = NULL;
    zval *zfrom_id = NULL;
    zval *zdata = NULL;
    MAKE_STD_ZVAL(zfd);
    MAKE_STD_ZVAL(zfrom_id);
    MAKE_STD_ZVAL(zdata);

    zval **args[4];
    zval *server_object = conn->GetThread()->tcpConnect->GetServerObject();
    args[0] = &server_object;
    ZVAL_LONG(zfd, (long)conn->GetFd());
    ZVAL_LONG(zfrom_id, (long)conn->GetThread()->tid);
    char *tmp = conn->GetAllReadBuffer();
    printf("%s", tmp);
    ZVAL_STRING(zdata, tmp, 1);
    efree(tmp);

    args[1] = &zfd;
    args[2] = &zfrom_id;
    args[3] = &zdata;
    zval *retval = NULL;
    zval *cb = zend_read_property(simple_server_class_entry_ptr, server_object, ZEND_STRL("onReceive"), 0 TSRMLS_CC);
    if (call_user_function_ex(EG(function_table), NULL, cb, &retval, 4, args, 0, NULL TSRMLS_CC) == FAILURE)
    {

    }
    if (EG(exception))
    {
        zend_exception_error(EG(exception), E_ERROR TSRMLS_CC);
    }
    zval_ptr_dtor(&zfd);
    zval_ptr_dtor(&zfrom_id);
    zval_ptr_dtor(&zdata);
    if (retval)
    {
        zval_ptr_dtor(&retval);
    }
}

static void simple_server_onclose(Conn *conn){
    zval *zfd = NULL;
    zval *zfrom_id = NULL;
    MAKE_STD_ZVAL(zfd);
    MAKE_STD_ZVAL(zfrom_id);

    zval **args[3];
    zval *server_object = conn->GetThread()->tcpConnect->GetServerObject();
    args[0] = &server_object;
    ZVAL_LONG(zfd, (long)conn->GetFd());
    ZVAL_LONG(zfrom_id, (long)conn->GetThread()->tid);
    args[1] = &zfd;
    args[2] = &zfrom_id;
    zval *retval = NULL;
    zval *cb = zend_read_property(simple_server_class_entry_ptr, server_object, ZEND_STRL("onClose"), 0 TSRMLS_CC);
    if (call_user_function_ex(EG(function_table), NULL, cb, &retval, 3, args, 0, NULL TSRMLS_CC) == FAILURE)
    {

    }
    if (EG(exception))
    {
        zend_exception_error(EG(exception), E_ERROR TSRMLS_CC);
    }
    zval_ptr_dtor(&zfd);
    zval_ptr_dtor(&zfrom_id);
    if (retval)
    {
        zval_ptr_dtor(&retval);
    }
}

static void simple_server_onconnect(Conn *conn){
    zval *zfd = NULL;
    zval *zfrom_id = NULL;
    MAKE_STD_ZVAL(zfd);
    MAKE_STD_ZVAL(zfrom_id);

    zval **args[3];
    zval *server_object = conn->GetThread()->tcpConnect->GetServerObject();
    args[0] = &server_object;
    ZVAL_LONG(zfd, (long)conn->GetFd());
    ZVAL_LONG(zfrom_id, (long)conn->GetThread()->tid);
    args[1] = &zfd;
    args[2] = &zfrom_id;
    zval *retval = NULL;
    zval *cb = zend_read_property(simple_server_class_entry_ptr, server_object, ZEND_STRL("onConnect"), 0 TSRMLS_CC);
    if (call_user_function_ex(EG(function_table), NULL, cb, &retval, 3, args, 0, NULL TSRMLS_CC) == FAILURE)
    {

    }
    if (EG(exception))
    {
        zend_exception_error(EG(exception), E_ERROR TSRMLS_CC);
    }
    zval_ptr_dtor(&zfd);
    zval_ptr_dtor(&zfrom_id);
    if (retval)
    {
        zval_ptr_dtor(&retval);
    }
}


void simple_server_init(int module_number TSRMLS_DC)
{
    INIT_CLASS_ENTRY(simple_server_ce, "simple_server", simple_server_methods);
    simple_server_class_entry_ptr = zend_register_internal_class(&simple_server_ce TSRMLS_CC);
    zend_declare_property_long(simple_server_class_entry_ptr,ZEND_STRL("port"),-1,ZEND_ACC_PUBLIC TSRMLS_CC);
    zend_declare_property_long(simple_server_class_entry_ptr,ZEND_STRL("threadNum"),2,ZEND_ACC_PUBLIC TSRMLS_CC);

    zend_declare_property_null(simple_server_class_entry_ptr,ZEND_STRL("onReceive"),ZEND_ACC_PUBLIC TSRMLS_CC);
    zend_declare_property_null(simple_server_class_entry_ptr,ZEND_STRL("onClose"),ZEND_ACC_PUBLIC TSRMLS_CC);
    zend_declare_property_null(simple_server_class_entry_ptr,ZEND_STRL("onConnect"),ZEND_ACC_PUBLIC TSRMLS_CC);
    zend_declare_property_null(simple_server_class_entry_ptr,ZEND_STRL("send"),ZEND_ACC_PUBLIC TSRMLS_CC);
}
