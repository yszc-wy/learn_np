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

