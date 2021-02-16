/*
* Author:           yszc-wy@foxmail.com 
* Encoding:			    utf-8
* Description:      管理accept获得的tcpconnection,根据acceptor提供的sockfd,将其注册到poller中,并创建tcpconnection,tcpconnection提供onConnection注册,还提供poller回调函数中的onMessage注册
* tcpserver由用户直接使用,生命周期由用户控制
*/
#pragma once

// C系统库头文件

// C++系统库头文件
#include <memory>
#include <string>
#include <map>
// 第三方库头文件
#include <boost/scoped_ptr.hpp>
// 本项目头文件
#include "base/noncopyable.h"
#include "Callback.h"




// 系统结构体或其他命名空间中类的前向声明(注意要包含相应的命名空间)


namespace yszc
{


namespace net{

// 本命名空间内部类的前向声明
class EventLoop;
class EventLoopThreadPool;
class Acceptor;
class InetAddress;

class TcpServer: noncopyable
{
 public:
  enum Option
  {
    kNoReusePort,
    kReusePort,
  };
  TcpServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg,
            Option option = kNoReusePort);
  ~TcpServer();

  const string& ipPort() const { return ipPort_; }
  const string& name() const { return name_; }
  EventLoop* getLoop() const { return loop_; }

  // 线程安全,多次调用无害
  void start();

  // 非线程安全
  void setConnectionCallback(const ConnectionCallback& cb)
  { connectionCallback_=cb; }
  // 非线程安全
  void setMessageCallback(const MessageCallback& cb)
  { messageCallback_=cb; }

  void setWriteCompleteCallback(const WriteCompleteCallback& cb)
  { writeCompleteCallback_=cb; }

  // 设置线程数量
  // 0意味着所有的I/O都在baseLoop中
  // 1意味着connection的I/O都在另外一个loop中
  // 2以后意味着connection的I/O都分布在另外2个loop中
  void setThreadNum(int numThreads);

 private:
  // 向acceptor注册
  void newConnection(int sockfd,const InetAddress& peerAddr);
  // 线程安全
  void removeConnection(const TcpConnectionPtr& conn);
  void removeConnectionInLoop(const TcpConnectionPtr& conn);
  typedef std::map<std::string,TcpConnectionPtr> ConnectionMap;

  EventLoop *loop_;  
  const string ipPort_;
  const std::string name_;
  boost::scoped_ptr<Acceptor> acceptor_;
  boost::scoped_ptr<EventLoopThreadPool> threadPool_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  bool started_;
  int nextConnId_;
  ConnectionMap connections_;
};
}

}

