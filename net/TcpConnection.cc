/*
* Author:           yszc-wy@foxmail.com * Encoding:			    utf-8
* Description:      
*/

// 本类头文件
#include "TcpConnection.h"
// C系统库头文件

// C++系统库头文件
#include <functional>
// 第三方库头文件
// 本项目头文件
#include "base/Logging.h"
#include "Channel.h"
#include "Socket.h"
#include "EventLoop.h"
#include "SocketsOps.h"

using namespace yszc;
using namespace yszc::net;


// 必须要指明时哪个命名空间的函数
void yszc::net::defaultConnectionCallback(const TcpConnectionPtr& conn)
{
  LOG_TRACE << conn->localAddress().toHostPort() << " -> "
            << conn->peerAddress().toHostPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");
  // do not call conn->forceClose(), because some users want to register message callback only.
}

void yszc::net::defaultMessageCallback(const TcpConnectionPtr&,
                                        Buffer* buf,
                                        Timestamp)
{
  buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop* loop,
                             const std::string& nameArg,
                             int sockfd,
                             const InetAddress& localAddr,
                             const InetAddress& peerAddr)
  : loop_(CHECK_NOTNULL(loop)),
    name_(nameArg),
    state_(kConnecting), // 初始状态为链接中
    socket_(new Socket(sockfd)),
    channel_(new Channel(loop, sockfd)),
    localAddr_(localAddr),
    peerAddr_(peerAddr)
{
  LOG_DEBUG << "TcpConnection::ctor[" <<  name_ << "] at " << this
            << " fd=" << sockfd;
  channel_->setReadCallback(
      std::bind(&TcpConnection::handleRead, this,std::placeholders::_1));
  channel_->setWriteCallback(
      std::bind(&TcpConnection::handleWrite, this));
  // 在channel中绑定了close和error回调
  channel_->setCloseCallback(
      std::bind(&TcpConnection::handleClose, this)); 
  channel_->setErrorCallback(
      std::bind(&TcpConnection::handleError, this));
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
  int savedErrno=0;
  // 使用inputBuffer_应用层缓冲,非阻塞必备
  // 必须一次性全部读完,否则会导致busy loop(LT模式的原因,如果接受缓冲区存在数据,就会一直触发epollwait返回)
  ssize_t n=inputBuffer_.readFd(channel_->fd(),&savedErrno);
  // char buf[65536];
  // ssize_t n=::read(channel_->fd(),buf,sizeof buf);
  if(n>0){
    messageCallback_(shared_from_this(),&inputBuffer_,receiveTime);
  } else if(n==0){
    // 在read回调中也有handleclose 和error回调
    // LOG_INFO<<"TcpConnection::handleRead::before handleClose";
    handleClose(); // 在这里处理
  } else {
    errno=savedErrno;
    LOG_SYSERR<<"TcpConnection::handleRead";
    handleError();
  }
}

void TcpConnection::handleClose()
{
  loop_->assertInLoopThread();
  LOG_TRACE<<"TcpConnection::handleClose state= "<<state_;
  // assert允许的状态转化
  assert(state_ == kConnected || state_ == kDisconnecting);
  channel_->disableAll();
  // 在CloseCallback执行过Tcpserver中的map删除操作后,除了shared_from_this以外,tcpconnection可能再无引用,导致该函数执行完毕后tcpconnection执行析构操作(tcpconnection使用shared_ptr且引用计数为0导致的),而tcpconnection的析构操作会导致其所持有的channel被析构,但handleClose处于该channel的handleEvent函数中的closecallback回调中,这使得handleevent函数在执行完closecallback后,其本身的对象被析构掉了,程序会崩溃
  // 简单来说就是在channel的回调中析构了channel对象(delete)本身
  // 为了让tcpconnection的生命周期延长到其所管理的channel handleevent函数执行完毕,需要将使用queueInloop函数将该conn对象的connectionDestoyed函数以及该对象本身注册到loop对象的唤醒回调队列中,以此唤醒loop并在当前channel的handleevent外的dependingfunctor中安全的removeChannel并析构整个tcpconnection和其附属的channel
  closeCallback_(shared_from_this());
}

void TcpConnection::handleError()
{
  int err=sockets::getSocketError(channel_->fd());
  LOG_ERROR << "TcpConnection::handleError [" << name_
            << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}

void TcpConnection::connectEstablished()
{
  loop_->assertInLoopThread();
  assert(state_ == kConnecting);
  // 设置状态为已连接
  setState(kConnected);
  // 建立连接时才注册poll
  channel_->enableReading();
  connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
  loop_->assertInLoopThread();
  assert(state_ == kConnected || state_ == kDisconnecting);
  // 设置为断开连接
  setState(kDisconnected);
  // 取消监听,但不删除
  channel_->disableAll();
  // 回调connectionCallback
  connectionCallback_(shared_from_this());
  // 删除Channel
  channel_->remove();
  // loop_->removeChannel(channel_.get());
}


void TcpConnection::shutdown()
{
  if(state_==kConnected)
  {
    setState(kDisconnecting);
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop,shared_from_this()));
  }
}

void TcpConnection::shutdownInLoop()
{
  loop_->assertInLoopThread();
  // 只有channel_中没有写事件监听,也就是没有暂缓的写任务时,才允许原地shutdown
  // 否则handleWrite函数会在判断处于disconnectiong状态后执行shutdownWrite
  if(!channel_->isWriting())
  {
    socket_->shutdownWrite();
  }
}

void TcpConnection::send(const std::string& message)
{
  // 只有在state_在kConnected情况下才允许发消息,要确定一个函数在什么状态下能才可以执行
  if(state_==kConnected){
    if(loop_->isInLoopThread()) {
      sendInLoop(message);
    } else {
      // 这里跨线程传输message时,到底是复制还是其他的?
      loop_->runInLoop(std::bind(&TcpConnection::sendInLoop,shared_from_this(),message));
    }
  }
}

void TcpConnection::send(Buffer* buf)
    
{
  if(state_==kConnected){
    if(loop_->isInLoopThread()) {
      sendInLoop(buf->retrieveAsString());
    } else {
      loop_->runInLoop(std::bind(&TcpConnection::sendInLoop,shared_from_this(),buf->retrieveAsString()));
    }
  }
}

void TcpConnection::sendInLoop(const std::string& message)
{
  loop_->assertInLoopThread();
  ssize_t nwrote=0;
  // 当前没有写任务在进行
  if(!channel_->isWriting() && outputBuffer_.readableBytes()==0){
    // 直接写
    nwrote=::write(channel_->fd(),message.data(),message.size());
    if(nwrote>=0){
      // 若数据没有写完,到下一个流程进行处理
      if(implicit_cast<size_t>(nwrote)<message.size()){
        LOG_TRACE<<"I am going to write more data";
      } 
      // 若已经写完,回调writecompletecallback
      else if(writeCompleteCallback_){
        // 为什么要重新插入队列运行,不能直接调用吗?
        loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
      }
    } else {
      nwrote=0;
      // 操作将被阻塞,一般是因为缓冲区不够,非阻塞编程常见
      if(errno!=EWOULDBLOCK){
        LOG_SYSERR<<"TcpConnection::sendInLoop";
      }
    }
  }

  // 如果已经处于writing状态,就不能直接写,而是将要写的数据放到outputbuff中,使用handread来处理
  // 如果遇到EWOULDBLOCK,或nwrote少于要写的数量,说明系统发送缓冲区不够了,需要等待
  // 我们就将剩下的数据放到outputbuff中,然后注册写事件,并用handread来帮助我们继续进行后续的发送操作
  assert(nwrote>=0);
  if(implicit_cast<size_t>(nwrote)<message.size()){

    size_t oldLen = outputBuffer_.readableBytes();
    // 获取剩余部分
    size_t remaining = message.size()-nwrote;

    // 超过水位线就触发回调
    if (oldLen + remaining >= highWaterMark_
        && oldLen < highWaterMark_
        && highWaterMarkCallback_)
    {
      // 这里就是经典的在dopendingfunction中注册cb的情况,这里queueInLoop要使用wakeup来唤醒下一次loop的poller来防止阻塞
      loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
    }

    // 由于函数只在一个线程执行,无需担心同步问题
    outputBuffer_.append(message.data()+nwrote,message.size()-nwrote);


    if(!channel_->isWriting()){
      channel_->enableWriting();
    }
  }
}

void TcpConnection::handleWrite()
{
  loop_->assertInLoopThread();
  if(channel_->isWriting()){
    // 同样,只调用一次,和之前只read一次理由相同
    ssize_t n= ::write(channel_->fd(),
                       outputBuffer_.peek(),
                       outputBuffer_.readableBytes());
    if(n>0){
      outputBuffer_.retrieve(n);
      // 写完毕
      if(outputBuffer_.readableBytes()==0){
        // 立即关闭防止busyloop,在水平触发模式下,只要发送缓冲区有空间就会触发该事件
        channel_->disableWriting();

        if(writeCompleteCallback_){
          // 为什么要重新插入队列运行,不能直接调用吗?
          loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
        }

        // 若处于disconnection状态,说明shutdown被调用
        if(state_==kDisconnecting){
          shutdownInLoop();
        }
      } else{
        LOG_TRACE<<"I am going to write more data";
      }
    } else {
      LOG_SYSERR<<"TcpConnection::handleWrite";
    }
  } else{
    LOG_TRACE<<"Connection is down, no more writing";
  }
}



TcpConnection::~TcpConnection()
{
  LOG_DEBUG << "TcpConnection::dtor[" <<  name_ << "] at " << this
            << " fd=" << channel_->fd();
}


void TcpConnection::forceClose()
{

  // FIXME: use compare and swap
  if (state_ == kConnected || state_ == kDisconnecting)
  {
    setState(kDisconnecting);
    loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
  }
}

void TcpConnection::forceCloseInLoop()
{
  loop_->assertInLoopThread();
  if (state_ == kConnected || state_ == kDisconnecting)
  {
    // as if we received 0 byte in handleRead();
    handleClose();
  }
}
