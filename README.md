# 简单web服务器
通读并注释了muduo核心代码,在此基础上运行了简单的web服务器,并进行压力测试


# 并发模式
  - TcpServer的EventLoop负责接受连接请求(Acceptor)
  - 使用EventLoopThreadPool线程池处理建立的每个连接,TcpServer线程会将Tcpconnection的处理工作(onMessage,send)分发给线程池中的一个EventLoopThread执行
  
# 相关技术点总结
- EventLoop 在单个线程内提供I/O事件的注册与回调,task注册(runInLoop,queueLoop),定时任务注册
- 每个连接中请求的处理使用Epoll水平触发,Acceptor主线程采用Epoll在水平触发的基础上循环Accept直到接受缓冲区清空,非阻塞I/O,Reactor,one loop per thread
- 基于(set)红黑树与timefd的EventLoop定时器
- 基于timingwheel关闭空闲连接
- EventLoop的task队列使用eventfd来实现异步唤醒EventLoop
- 多线程发挥多核cpu的优势,让Acceptor的I/O操作与TcpConnection的I/O操作在不同的线程中并行运行
- 4缓冲异步日志,日志系统实现前后端,可以自由选择后端的输出函数,异步日志系统不会阻塞主线程
- TcpServer的EventLoop与EventLoopThread之间使用task任务队列来向其他线程分派任务,并以此避免多线程下的对象访问的出现的锁争用,一个tcpconnection的发送和接收操作只会在该conn所在的线程执行
- TcpConnection的建立与析构过程就需要TcpServer Eventloop与线程池中EventLoop之间相互协调
![avatar](https://www.notion.so/image/https%3A%2F%2Fs3-us-west-2.amazonaws.com%2Fsecure.notion-static.com%2Fced55428-6057-4863-a35d-e59edd89ee41%2FUntitled.png?table=block&id=9fcc6120-46db-4070-8682-19cf032204a3&spaceId=1c520d5d-549c-43bf-9033-ff578b668b8c&width=1420&userId=978aa85b-e106-44f4-bf74-16b7b995b454&cache=v2)
- 对于一些需要大量计算的连接,为了避免计算操作导致降低I/O响应速度,我们可以使用线程池来执行计算任务,当任务执行完毕再使用runInLoop在TcpConnection中回调TaskComplete函数
[avatar](https://www.notion.so/image/https%3A%2F%2Fs3-us-west-2.amazonaws.com%2Fsecure.notion-static.com%2F4f0aa015-104b-4dad-99ad-65edd89b64a0%2FUntitled.png?table=block&id=9080001f-fba4-44ff-a230-427765ad6516&spaceId=1c520d5d-549c-43bf-9033-ff578b668b8c&width=1260&userId=978aa85b-e106-44f4-bf74-16b7b995b454&cache=v2)

- ET与LT在输入输出上的区别

```
LT模式需要读完的原因时缓冲区不为空会busy loop，比如对于listenfd，缓冲区中有多个fd等待accept,但muduo就只accept一次，下次epoll_wait 还会返回，因为listenfd缓冲区中还有连接没有完全读完，其实muduo的readbf也只执行了一次读取操作（使用了栈上空间，一次能读取的数量很多），但并不保证读完，若没有读完的会在下一次epoll_wait中触发。
LT模式在输出缓冲区为空时需要及时取消监听写是因为只要发送缓冲区还有剩余空间epoll_wait就会返回，造成busy loop.
ET模式需要全部读完的原因是，如果不读完，缓冲区在已经有数据的情况下再来新数据是不会通知的，最终会导致缓冲区爆满，但epoll_wait却迟迟不返回
ET模式下发送数据，在writeBuffer不为空的情况下，一定要将Buffer中的内容填满写缓冲，如果writeBuffer不为空，又不讲fd的写缓冲写满，会导致下一次写事件一直不被触发，无法清空writeBuffer.不像LT模式需要及时取消注册，ET模式放在那里不管也行，因为写缓冲为空不会触发epoll_wait. ET模式在发送数据时要遵循先write,如果写满fd缓冲还不能完全发送数据，才注册事件，这样ET模式才可以监听到写缓冲区不满的通知
```
- http请求解析以行为单位解析,每当接受缓冲区接收到一行数据,就根据HttpContext的当前状态进行对应行的解析,将解析结果保存在HttpRequest中,解析完毕后调用onRequest,通过header中的Connection来决定是否在onRequest函数中执行shutdown

# 测试
[测试](https://github.com/yszc-wy/learn_np/blob/master/server/test.md)
