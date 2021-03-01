/*
* Encoding:			    utf-8
* Description:      本身由shared_ptr管理生命周期,包含Socket对象的unique_ptr,因此负责管理connection的sockfd析构,自己处理writeable事件,为readable事件提供回调接口
* 创建:connection由使用者在连接建立获取到fd时创建
* 销毁:Tcpconnection在主动关闭和被动关闭时并没有调用销毁(connectDestroyed)操作,但tcpconnection在handleClose时会调用closecallback,可以向其中注册包含connectDestroyed来实现连接的销毁操作,注意connectionDestroyed只能在doPendingFunctors中运行,所以要在closecallback中要使用queueinloop注册connectDestroyed
* 比如:tcpserver向tcpconnection中注册closecallback来删除server map中指定的connectionptr,并对该ptr调用queueinloop来调用
*/
#pragma once

// C系统库头文件

// C++系统库头文件
#include <memory>
// 第三方库头文件
#include <boost/any.hpp>
// 本项目头文件
#include "base/noncopyable.h"
#include "base/Timestamp.h"
#include "InetAddress.h"
#include "Callback.h"
#include "Buffer.h"
#include "net/TimeWheel.h"



// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{


namespace net
{
class EventLoop;
class Channel;
class Socket;

class TcpConnection : noncopyable,
                      public std::enable_shared_from_this<TcpConnection>
{
 public: 

  /// Constructs a TcpConnection with a connected sockfd
  /// User should not create this object.
  TcpConnection(EventLoop* loop,
                const std::string& name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);
  ~TcpConnection();
  // 读接口
  EventLoop* getLoop() const { return loop_; }
  const std::string& name() const { return name_; }
  const InetAddress& localAddress() { return localAddr_; }
  const InetAddress& peerAddress() { return peerAddr_; }
  bool connected() const { return state_ == kConnected; }

  void setTimeWheelWeakEntry(const TimeWheel::WeakEntryPtr& weak_ptr) 
  { weak_entry_ptr_context_=weak_ptr;  }
  
  const TimeWheel::WeakEntryPtr& getTimeWheelWeakEntry()
  { return weak_entry_ptr_context_;}

  void setTimeWheelTail(TimeWheel::BucketList::iterator tail)
  { tail_=tail; }
  TimeWheel::BucketList::iterator getTimeWheelTail() 
  { return tail_; }

  void setContext(const boost::any& context)
  { context_ = context; }

  const boost::any& getContext() const
  { return context_; }

  boost::any* getMutableContext()
  { return &context_; }

  
  void setConnectionCallback(const ConnectionCallback& cb)
  { connectionCallback_ = cb; }

  void setMessageCallback(const MessageCallback& cb)
  { messageCallback_ = cb; }

  void setCloseCallback(const CloseCallback &cb)
  {  closeCallback_= cb; }
  
  void setWriteCompleteCallback(const WriteCompleteCallback& cb)
  { writeCompleteCallback_=cb; }

  void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
  { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

  // void send(const void* message,size_t len);
  // Thread safe,可以在别的线程调用这2个函数,然后使用*inLoop函数使其在自己的loop中执行
  void send(const std::string& message);
  void send(Buffer* buf);
  void shutdown();

  void forceClose();
  // Tcp选项
  void setTcpNoDelay(bool on);

  // called when TcpServer accepts a new connection
  void connectEstablished();   // should be called only once ,connectionCallback_
  // called when TcpServer has removed me from its map
  void connectDestroyed();  // // should be called only once
 private:
  enum StateE { kConnecting, kConnected, kDisconnecting,kDisconnected, };
  void setState(StateE s){ state_=s; }
  void handleRead(Timestamp receiveTime); // 处理消息到达 messageCallback_
  void handleWrite();
  void handleClose(); 
  void handleError();
  // 为了线程安全考虑
  void sendInLoop(const std::string& message);
  void shutdownInLoop();
  void forceCloseInLoop();
  EventLoop* loop_;
  std::string name_;
  StateE state_; // atomic
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;
  InetAddress localAddr_;
  InetAddress peerAddr_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  HighWaterMarkCallback highWaterMarkCallback_;
  size_t highWaterMark_;
  CloseCallback closeCallback_;
  Buffer inputBuffer_;
  Buffer outputBuffer_;
  boost::any context_;
  TimeWheel::WeakEntryPtr weak_entry_ptr_context_;
  TimeWheel::BucketList::iterator tail_;
};


void defaultConnectionCallback(const TcpConnectionPtr& conn);

void defaultMessageCallback(const TcpConnectionPtr&,
                                        Buffer* buf,
                                        Timestamp);

}

}

