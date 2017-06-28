# simple_server
练习PHP扩展编写 借助libevent实现一个reactor驱动的tcp服务  
主进程依赖libevent实现reactor多线程收发tcp消息  
多个子进程(worker进程)接收reactor线程投递的消息，按照用户指定的回调函数处理后，返回给reactor线程，最终输出给客户端

**只是作为练习，没有考虑异常情况，以跑通为目的 :)**
```php
<?php
$s = new simple_server(9981, 2, 2);

$s->on("connect", function($serv, $fd, $td){
   // var_dump($serv, $fd, $td);
   echo "connected\n";
});

$s->on("close", function($serv, $fd, $td){
    //var_dump($serv, $fd, $td);
    echo "closed\n";
});

//实现将客户端输入的整数+1后输出给客户端
$s->on("receive", function($serv,$fd,$td, $data){
    echo "PHP receive:".($data);
    if(is_numeric(trim($data))){
        $serv->send($fd, (intval(trim($data))+1)."\n");
    }
});

$s->start();
```

```php
telnet 127.0.0.1 9981
```
