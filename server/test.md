# 测试

## 环境
model name      : Intel(R) Core(TM) i7-4720HQ CPU @ 2.60GHz
cpu cores       : 4
MemTotal: 8G
## 测试方法

- 水平触发,单线程, GET hello,release

测试结果

```
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://127.0.0.1:8000/hello (using HTTP/1.1)
1000 clients, running 60 sec.

Speed=1590195 pages/min, 2437405 bytes/sec.
Requests: 1589614 susceed, 581 failed.
QPS 26493
```
出现错误
```
20210216 03:00:07.205918Z  9028 INFO  TcpServer::newConnection [dummy] - new connection [dummy#1588728] from 127.0.0.1:46308 - TcpServer.cc:104   
20210216 03:00:07.205998Z  9028 ERROR Transport endpoint is not connected (errno=107) sockets::shutdownWrite - SocketsOps.cc:340
20210216 03:00:07.206016Z  9028 ERROR TcpConnection::handleError [dummy#1588728] - SO_ERROR = 32 Broken pipe - TcpConnection.cc:102
20210216 03:00:07.206021Z  9028 INFO  TcpServer::removeConnection [dummy] - connectiondummy#1588728 - TcpServer.cc:82
```
webbench终止client,发送ret请求,shutdownWrite被传入接收了ret的socket,handleError被触发
(当我们往一个对端已经close的通道写数据的时候，对方的tcp会收到这个报文，并且反馈一个reset报文，tcpdump的结果如下所示,当收到reset报文的时候，继续做select读数据的时候就会抛出Connect reset by peer的异常，从堆栈可以看得出,[接受到reset]

当第一次往一个对端已经close的通道写数据的时候会和上面的情况一样，会收到reset报文，当再次往这个socket写数据的时候，就会抛出Broken pipe了 ，根据tcp的约定，当收到reset包的时候，上层必须要做出处理，调用将socket文件描述符进行关闭，其实也意味着pipe会关闭，因此会抛出这个顾名思义的异常 [对reset的fd进行了写操作]
)

- 水平触发,4线程,GET hello,release
测试结果

```
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://127.0.0.1:8000/hello (using HTTP/1.1)
1000 clients, running 60 sec.

Speed=4471065 pages/min, 6855631 bytes/sec.
Requests: 4471064 susceed, 1 failed.
QPS 74517
```

系统负载
```
 进程 USER      PR  NI    VIRT    RES    SHR    %CPU  %MEM     TIME+ COMMAND                                                                      
10635 yszc      20   0  306812  10768   3644 R  76.7   0.1   0:20.36 server                                                                       
10637 yszc      20   0  306812  10768   3644 R  49.8   0.1   0:12.89 Thread2                                                                      
10636 yszc      20   0  306812  10768   3644 S  48.8   0.1   0:12.89 Thread1                                                                      
10639 yszc      20   0  306812  10768   3644 S  48.8   0.1   0:12.95 Thread4                                                                      
10638 yszc      20   0  306812  10768   3644 S  48.5   0.1   0:12.97 Thread3 
```



- 水平触发,8线程,GET hello, release
测试结果

```
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://127.0.0.1:8000/hello (using HTTP/1.1)
1000 clients, running 60 sec.

Speed=3955314 pages/min, 6064807 bytes/sec.
Requests: 3955311 susceed, 3 failed.
```

```
 进程 USER      PR  NI    VIRT    RES    SHR    %CPU  %MEM     TIME+ COMMAND                                                                      
 4521 yszc      20   0  602504  11672   3444 R  80.0   0.1   0:28.48 server                                                                       
 4523 yszc      20   0  602504  11672   3444 R  23.0   0.1   0:08.19 Thread2                                                                      
 4526 yszc      20   0  602504  11672   3444 S  23.0   0.1   0:08.18 Thread5                                                                      
 4528 yszc      20   0  602504  11672   3444 R  23.0   0.1   0:08.19 Thread7                                                                      
 4529 yszc      20   0  602504  11672   3444 S  23.0   0.1   0:08.18 Thread8                                                                      
 4524 yszc      20   0  602504  11672   3444 R  22.7   0.1   0:08.19 Thread3                                                                      
 4527 yszc      20   0  602504  11672   3444 R  22.7   0.1   0:08.17 Thread6                                                                      
 4522 yszc      20   0  602504  11672   3444 R  22.3   0.1   0:08.17 Thread1                                                                      
 4525 yszc      20   0  602504  11672   3444 S  22.3   0.1   0:08.18 Thread4 
```


- 水平触发,4线程,GET hello,release,accept采用循环accept(避免busy loop)
测试结果

```
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://127.0.0.1:8000/hello (using HTTP/1.1)
1000 clients, running 60 sec.

Speed=4579374 pages/min, 7021702 bytes/sec.
Requests: 4579374 susceed, 0 failed.
QPS 76322
```
- 边沿触发,4线程,GET hello, release

```
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://127.0.0.1:8000/hello (using HTTP/1.1)
1000 clients, running 60 sec.

Speed=4539174 pages/min, 6960064 bytes/sec.
Requests: 4539172 susceed, 2 failed.
```


系统空闲负载
```
 进程 USER      PR  NI    VIRT    RES    SHR    %CPU  %MEM     TIME+ COMMAND                                                                      
18669 yszc      20   0  306972  11480   3320 S   0.0   0.1   5:31.23 server 
```

