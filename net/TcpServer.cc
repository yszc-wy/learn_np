/*
* Author:           
* Encoding:			    utf-8
* Description:      
*/

// 本类头文件
#include "TcpServer.h"
// C系统库头文件

// C++系统库头文件
// 第三方库头文件
#include "base/Logging.h"
// 本项目头文件
#include "Acceptor.h"
#include "InetAddress.h"
#include "EventLoop.h"
#include "SocketsOps.h"
#include "TcpConnection.h"
#include "EventLoopThreadPool.h"

using namespace yszc;
using namespace yszc::net;


TcpServer::TcpServer(EventLoop* loop,
                     const InetAddress& listenAddr,
                     const string& nameArg,
                     Option option)
  : loop_(loop),
    ipPort_(listenAddr.toHostPort()),
    name_(nameArg),
    acceptor_(new Acceptor(loop,listenAddr,option==kReusePort)),
    threadPool_(new EventLoopThreadPool(loop)),
    connectionCallback_(defaultConnectionCallback),
    messageCallback_(defaultMessageCallback),
    started_(false),
    nextConnId_(1)
{
  using namespace std::placeholders;
  acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection,this,_1,_2));
}

TcpServer::~TcpServer()
{
}

void TcpServer::setThreadNum(int numThreads)
{
  assert(0 <= numThreads);
  threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
  if(!started_)
  {
    started_=true;
    threadPool_->start();
  }

  if(!acceptor_->listenning())
  {
    // 这里向loop传入了野指针,如何防止loop访问到已经析构的acceptor指针,就是如果TcpServer的生命周期比loop短,loop在loop()函数中可能访问到空的accepter指针(不过大部分情况下TcpServer与loop的生命周期相同,除非TcpServer与loop在不同线程中) 因为loop对象的生命周期与TcpServer的生命周期无关
    loop_->runInLoop(
                     std::bind(&Acceptor::listen,acceptor_.get()));
  }
  
}
// 调用removeConnection的可能是处于其他线程的tcpconnection所注册的回调,问题是removeConnection函数不是线程安全的,且此时loop_不再connection所在的线程中,所以要通过runInloop包裹原来的removeConnection,来实现线程安全的removeConnection,该函数可以被任意线程调用
// 另外TcpConnection的connectDestoryed却需要在connection注册的loop线程中销毁,所以需要先获得Tcpconnection的loop指针,然后runInLoop (这也是为何之前这里用queueInLoop的原因,因为TcpServer和TcpConnection可能不再同一个线程中)
void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
  // 为了重新回到main_io_loop中运行从map中删除tcpconnection
  loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop,this,conn));
}

// tcpserver->管理-> connects, connects在析构时回调的执行removeConnection的清理操作
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
  loop_->assertInLoopThread();
  LOG_INFO<<"TcpServer::removeConnection ["<<name_
    <<"] - connection"<<conn->name();
  // 删除map中的connect,引用计数下降
  size_t n=connections_.erase(conn->name());
  assert(n==1); (void)n;
  // 这里为何不用runinloop? 防止在channel的handleEvent中调用connectionDestory,从而调用channel remove,这使得channel在执行handleevent过程中被析构了,这会导致系统异常
  // 由于这时tcpconnection已经命悬一线,如果用户不持有conn的ptr的话,在该函数执行完毕后tcpconnectionptr的引用计数会下降到0,然后被析构,但我们需要将conn的生命周期延长到该函数调用后,因为我们要在doPendingFunctor中调用connectDestroyed进行清理工作
  // 也就意味着对于TcpServer来说我们必须要调用connectDestroyed函数,因为如果不调用该函数会导致tcpconection在该channel函数运行时析构,造成错误 
  // bind让conn的生命周期延长到调用connectDestroyed()的时刻,也就是在handleEvent的外部: doPendingFunctors中回调该函数
  EventLoop* ioLoop=conn->getLoop();
  ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed,conn));
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
  loop_->assertInLoopThread();
  char buf[32];
  snprintf(buf, sizeof buf, "#%d", nextConnId_);
  ++nextConnId_;
  // 连接名使用servername结合nextConnId
  std::string connName = name_ + buf;

  LOG_INFO << "TcpServer::newConnection [" << name_
           << "] - new connection [" << connName
           << "] from " << peerAddr.toHostPort();

  // 创建Tcpconnection对象,并注册相应的函数,与sockfd有关的所有消息回调都由该连接管理
  // Tcpconnection对象的生命周期由其引用计数管理,以防止sockfd在使用时被另一个线程析构
  InetAddress localAddr(sockets::getLocalAddr(sockfd));
  // 若为0就返回base_loop
  EventLoop* ioLoop = threadPool_->getNextLoop();
  // FIXME poll with zero timeout to double confirm the new connection
  // 用不同线程的loop负责相应的TcpConnection,这时TcpConnection很多函数都是跨线程调用loop,要保证loop的线程安全性(TcpConnection可能在主线程中被用户所管理,但其loop是运行在另外的线程上的)
  TcpConnectionPtr conn(
      new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
  connections_[connName] = conn; // 注册连接

  // 可以看出一个TcpServer下的所有连接注册的connectionCallback和messageCallback都相同
  // set系列的回调是线程安全的
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  // TcpServer向conn注册closecallback回调,这个回调由Tcpserver自己定义,析构操作与创建操作对应,都是在loop_中执行
  conn->setCloseCallback(std::bind(&TcpServer::removeConnection,this,std::placeholders::_1));
  // Tcpconnect除了new和注册操作以外的全部操作都在ioLoop中执行
  ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished,conn));
}


void TcpConnection::setTcpNoDelay(bool on)
{
  socket_->setTcpNoDelay(on);
}
