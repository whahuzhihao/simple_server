# simple_server
练习PHP扩展编写 借助libevent实现一个reactor驱动的tcp服务

```php
<?php
$s = new simple_server(9981, 2);
$s->on("connect", function($serv, $fd, $td){
   // var_dump($serv, $fd, $td);
   echo "connect\n";
});

$s->on("close", function($serv, $fd, $td){
    //var_dump($serv, $fd, $td);
    echo "closed\n";
});

$s->on("receive", function($serv,$fd,$td, $data){
    var_dump($data);
    if(trim($data) == "send")
    $serv->send($fd, "sendResult");
});

$s->start();
```

```php
telnet 127.0.0.1 9981
```
